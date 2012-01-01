#include <boost/foreach.hpp>

#include "DependencyResolutionException.h"
#include "InstanceLookup.h"
#include "ISharingLifetimeScope.h"
#include "ResolveOperation.h"


namespace Hypodermic
{

    ResolveOperation::ResolveOperation(ISharingLifetimeScope* mostNestedLifetimeScope)
        : mostNestedLifetimeScope_(mostNestedLifetimeScope)
        , callDepth_(0)
        , ended_(false)
    {
        if (mostNestedLifetimeScope == nullptr)
            throw std::invalid_argument("mostNestedLifetimeScope");
        resetSuccessfulActivations();
    }

    IComponentRegistry* ResolveOperation::componentRegistry()
    {
        return mostNestedLifetimeScope_->componentRegistry();
    }

    void* ResolveOperation::resolveComponent(IComponentRegistration* registration)
    {
        return getOrCreateInstance(mostNestedLifetimeScope_, registration);
    }

    void* ResolveOperation::execute(IComponentRegistration* registration)
    {
        void* result = nullptr;

        try
        {
            result = resolveComponent(registration);
        }
        catch (DependencyResolutionException& ex)
        {
            end(ex);
            throw;
        }
        catch (std::exception& ex)
        {
            end(ex);
            throw DependencyResolutionException(ex.what());
        }

        end();
        return result;
    }

    void* ResolveOperation::getOrCreateInstance(ISharingLifetimeScope* currentOperationScope,
                                                IComponentRegistration* registration)
    {
        if (currentOperationScope == nullptr)
            throw std::invalid_argument("currentOperationScope");
        if (registration == nullptr)
            throw std::invalid_argument("registration");
        //if (ended_)
        //    throw ObjectDisposedException(ResolveOperationResources.TemporaryContextDisposed, innerException: null);

        circularDependencyDetector_.checkForCircularDependency(registration, activationStack_, ++callDepth_);

        auto activation = new InstanceLookup(registration, this, currentOperationScope);

        activationStack_.push_back(activation);

        //InstanceLookupBeginning(this, new InstanceLookupBeginningEventArgs(activation));

        auto instance = activation->execute();
        successfulActivations_.push_back(activation);

        activationStack_.pop_back();

        //if (activationStack_.size() == 0)
        //    CompleteActivations();

        --callDepth_;

        return instance;
    }

    void ResolveOperation::completeActivations()
    {
        auto completedActivations = successfulActivations_;
        resetSuccessfulActivations();

        BOOST_FOREACH(auto activation, completedActivations)
            activation->complete();
    }

    void ResolveOperation::resetSuccessfulActivations()
    {
        successfulActivations_.clear();
    }

    void ResolveOperation::end(std::exception& exception)
    {
        if (!ended_)
        {
            ended_ = true;
            //CurrentOperationEnding(this, new ResolveOperationEndingEventArgs(this, exception));
        }
    }

    void ResolveOperation::end()
    {
        if (!ended_)
        {
            ended_ = true;
            //CurrentOperationEnding(this, new ResolveOperationEndingEventArgs(this));
        }
    }

} // namespace Hypodermic