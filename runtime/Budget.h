// Declares BudgetExceeded and RAII BudgetScope for limiting runtime statement execution counts.
#ifndef BUDGET_H
#define BUDGET_H

#include <exception>

namespace gash{
    class BudgetScope;

    // Exception thrown when an active BudgetScope reaches zero remaining operations.
    class BudgetExceeded : public std::exception {
    public:
        // Records which BudgetScope triggered exhaustion so nested scopes unwind correctly.
        explicit BudgetExceeded(
            const BudgetScope* target
        ) noexcept : target_(target) {}

        // Returns the BudgetScope instance responsible for this exhaustion exception.
        const BudgetScope* target() const noexcept {
            return target_;
        }

        // Returns a human-readable budget exhaustion message.
        const char* what() const noexcept override {
            return "GASH execution budget exhausted";
        }

    private:
        const BudgetScope* target_;
    };

    // RAII guard tracking operation ticks across nested budget blocks.
    class BudgetScope {
    public:
        // Activates a new budget scope with the given non-negative operation limit.
        explicit BudgetScope(long long limit);
        // Pops this budget scope from the thread-local active budget stack.
        ~BudgetScope() noexcept;

        BudgetScope(const BudgetScope&) = delete;
        BudgetScope& operator=(const BudgetScope&) = delete;

        // Verifies all active budgets (outermost first) still have remaining steps.
        static void check();
        // Checks active budgets and charges one execution step against every active budget.
        static void tick();

        // Returns true if this BudgetScope is the target that threw the exception.
        bool handles(const BudgetExceeded& error) const noexcept;

    private:
        long long remaining_;
    };
};
#endif