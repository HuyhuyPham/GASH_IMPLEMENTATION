#include "CodeGenerator.h"

#include <iomanip>
#include <stdexcept>
#include <iostream>

namespace gash{
    // Formatting
    void CodeGenerator::emitLine(const std::string &line){
        for(int i = 0; i < indentLevel; i++){
            output << "    ";
        }
        output << line << std::endl;
    }
    // Statement generation
    void CodeGenerator::emitStatement(const Stmt &stmt){
        if (const auto* decl = dynamic_cast<const VarDeclStmt*>(&stmt)) {
            if (!decl->initializer) {
                throw std::runtime_error("Variable declaration requires an initializer");
            }
            emitLine("auto gash_" + decl->name + " = " + emitExpression(*decl->initializer) + ";");
            return;
        }
        if (const auto* exprStmt = dynamic_cast<const ExpressionStmt*>(&stmt)) {
            if (const auto* call = dynamic_cast<const CallExpr*>(exprStmt->expression.get())) {
                if (call->callee == "output") {
                    if (call->arguments.size() != 1) {
                        throw std::runtime_error("Initial output() implementation requires one argument");
                    }
                    emitLine("std::cout << " +emitExpression(*call->arguments[0]) +" << '\\n';");
                    return;
                }
            }
            emitLine(emitExpression(*exprStmt->expression) + ";");
            return;
        }
        throw std::runtime_error("Statement type not yet supported by CodeGenerator");
    }
    // Expression generation
    std::string CodeGenerator::emitExpression(const Expr &expr) const{
        if (const auto* node = dynamic_cast<const IntegerExpr*>(&expr)) {
            return std::to_string(node->value);
        }
        if (const auto* node = dynamic_cast<const FloatExpr*>(&expr)) {
            std::ostringstream value;
            value << std::setprecision(17) << node->value;
            return value.str();
        }
        if (const auto* node = dynamic_cast<const BoolExpr*>(&expr)) {
            if(node->value == true){
                return "true";
            }
            return "false";
        }
        if (const auto* node = dynamic_cast<const StringExpr*>(&expr)) {
            return "\"" + escapeString(node->value) + "\"";
        }
        if (const auto* node = dynamic_cast<const VariableExpr*>(&expr)) {
            return "gash_" + node->name;
        }
        if (const auto* node = dynamic_cast<const BinaryExpr*>(&expr)) {
            if (node->op == "^") {
                throw std::runtime_error("Exponentiation code generation not implemented yet");
            }
            return "(" + emitExpression(*node->left) +" " + node->op + " " +emitExpression(*node->right) +")";
        }
        if (const auto* node = dynamic_cast<const UnaryExpr*>(&expr)) { 
            return "(" + node->op + emitExpression(*node->operand) +")";
        }
        throw std::runtime_error("Expression type not yet supported by CodeGenerator");
    }
    // Escape a GASH string for C++ output.
    std::string CodeGenerator::escapeString(const std::string &value){
        std::string result;
        for(int i = 0; i < value.size(); i++){
            if(value[i] == '\\'){
                result += "\\\\";
            } else if (value[i] == '""'){
                result += "\\\"";
            } else if(value[i] == '\n'){
                result += "\\n";
            } else if(value[i] == '\t'){
                result += "\\t";
            } else if(value[i] == '\r'){
                result += "\\r";
            } else {
                result += value[i];
            }
        }
        return result;
    }
    std::string CodeGenerator::generate(const Program &program){
        indentLevel = 0;
        output.str("");
        output.clear();

        emitLine("#include <iostream>");
        emitLine("#include <string>");
        emitLine("");

        emitLine("int main(){");
        indentLevel++;

        for (const auto& stmt : program.statements) {
            emitStatement(*stmt);
        }

        emitLine("return 0;");

        indentLevel--;
        emitLine("}");

        return output.str();
    }
}