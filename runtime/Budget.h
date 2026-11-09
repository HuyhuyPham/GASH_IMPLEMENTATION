#ifndef BUDGET_H
#define BUDGET_H

#include <exception>

namespace gash{
    class BudgetScope;

    class BudgetExceeded : public std::exception {
    public:
        explicit BudgetExceeded(
            const BudgetScope* target
        ) noexcept : target_(target) {}

        const BudgetScope* target() const noexcept {
            return target_;
        }

        const char* what() const noexcept override {
            return "GASH execution budget exhausted";
        }

    private:
        const BudgetScope* target_;
    };

    class BudgetScope {
    public:
        explicit BudgetScope(long long limit);
        ~BudgetScope() noexcept;

        BudgetScope(const BudgetScope&) = delete;
        BudgetScope& operator=(const BudgetScope&) = delete;

        static void check();
        static void tick();

        bool handles(const BudgetExceeded& error) const noexcept;

    private:
        long long remaining_;
    };
};
#endif