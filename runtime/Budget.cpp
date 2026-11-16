
// Implements BudgetScope stack management and step accounting for `budget(...)` blocks.

#include "Budget.h"

#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
    thread_local std::vector<gash::BudgetScope *> activeBudgets;
}

namespace gash
{

    // Validates non-negative limit and pushes this scope onto activeBudgets.
    BudgetScope::BudgetScope(long long limit)
        : remaining_(limit)
    {

        if (limit < 0)
        {
            throw std::invalid_argument(
                "Budget cannot be negative");
        }

        activeBudgets.push_back(this);
    }

    // Pops this scope from activeBudgets upon block exit or stack unwinding.
    BudgetScope::~BudgetScope() noexcept
    {
        if (activeBudgets.empty() ||
            activeBudgets.back() != this)
        {
            std::terminate();
        }

        activeBudgets.pop_back();
    }

    // Throws BudgetExceeded for the outermost exhausted budget if any remaining_ == 0.
    void BudgetScope::check()
    {
        // Check outermost budget first.
        for (BudgetScope *budget : activeBudgets)
        {
            if (budget->remaining_ == 0)
            {
                std::cout.flush();
                std::cerr
                    << "Warning: execution budget exhausted\n";

                throw BudgetExceeded(budget);
            }
        }
    }

    // Checks remaining budgets and decrements every active budget by one step.
    void BudgetScope::tick()
    {
        // Prevent an operation if any budget is exhausted.
        check();

        // Charge one operation to every active budget.
        for (BudgetScope *budget : activeBudgets)
        {
            --budget->remaining_;
        }
    }

    // Returns whether `error` originated from this specific BudgetScope.
    bool BudgetScope::handles(
        const BudgetExceeded &error) const noexcept
    {
        return error.target() == this;
    }
}