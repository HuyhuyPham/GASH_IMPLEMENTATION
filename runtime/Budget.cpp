
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

    BudgetScope::~BudgetScope() noexcept
    {
        if (activeBudgets.empty() ||
            activeBudgets.back() != this)
        {
            std::terminate();
        }

        activeBudgets.pop_back();
    }

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

    bool BudgetScope::handles(
        const BudgetExceeded &error) const noexcept
    {
        return error.target() == this;
    }
}