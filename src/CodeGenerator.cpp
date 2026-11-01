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
        if(const auto* blockStmt = dynamic_cast<const BlockStmt*>(&stmt)){
            emitLine("{");
            indentLevel++;
            for(int i = 0; i < blockStmt->statements.size(); i++){
                emitStatement(*blockStmt->statements[i]);
            }
            indentLevel--;
            emitLine("}");
            return;
        }
        if(const auto* ifStmt = dynamic_cast<const IfStmt*>(&stmt)){
            std::string condition = emitExpression(*ifStmt->condition);
            emitLine("if(" + condition + ")");
            emitStatement(*ifStmt->thenBranch);
            if (ifStmt->elseBranch != nullptr) {
                emitLine("else");
                emitStatement(*ifStmt->elseBranch);
            }
            return;
        }
        if(const auto* whileStmt = dynamic_cast<const WhileStmt*>(&stmt)){
            std::string condition = emitExpression(*whileStmt->condition);
            emitLine("while(" + condition + ")");
            emitStatement(*whileStmt->body);
            return;
        }
        if(const auto* repeatStmt = dynamic_cast<const RepeatStmt*>(&stmt)){
            std::string times = emitExpression(*repeatStmt->count);
            int id = nextRepeatId++;
            std::string counterName = "gash_repeat_index_" + std::to_string(id);
            std::string limitName = "gash_repeat_limit_" + std::to_string(id);
            emitLine("{");
            indentLevel++;
            emitLine("const int " + limitName + " = " + times + ";");
            emitLine("for (int " + counterName + " = 0; " +counterName + " < " + limitName + ";" + counterName + "++)");
            emitStatement(*repeatStmt->body);
            emitLine("}");
            return;
        }
        if(const auto* exitStmt = dynamic_cast<const ExitStmt*>(&stmt)){
            emitLine("break;");
            return;
        }
        if(const auto* skipStmt = dynamic_cast<const SkipStmt*>(&stmt)){
            emitLine("continue;");
            return;
        }
        if(const auto* returnStmt = dynamic_cast<const ReturnStmt*>(&stmt)){
            if (returnStmt->value == nullptr) {
                emitLine("return;");
            } else {
                std::string value = emitExpression(*returnStmt->value);
                emitLine("return " + value + ";");
            }
            return;
        }
        if(const auto* functionStmt = dynamic_cast<const FunctionDeclStmt*>(&stmt)){
            std::string name = "gash_" + functionStmt->name;
            std::string signature = "auto " + name + "(";
            for(size_t i = 0; i < functionStmt->parameters.size(); i++){
                const auto& param = functionStmt->parameters[i];
                std::string cppType = mapType(param.type);
                if (i > 0) {
                    signature += ", ";
                }
                signature += cppType + " gash_" + param.name;
            }
            signature += ")";
            emitLine(signature);
            emitStatement(*functionStmt->body);
            return;
        }
        if(const auto* expressionStmt = dynamic_cast<const ExpressionStmt*>(&stmt)){
            if(const auto* callExpr = dynamic_cast<const CallExpr*>(expressionStmt->expression.get())){
                if(callExpr->callee == "output"){
                    int count = callExpr->arguments.size();
                    if(count != 1){
                        throw std::runtime_error("output() requires exactly one argument");
                    }
                    std::string argument = emitExpression(*callExpr->arguments[0]);
                    emitLine("std::cout << " + argument + " << '\\n;'");
                    return;
                }
            }
            emitLine(emitExpression(*expressionStmt->expression) + ";");
            return;
        }
        if(const auto* expressionStmt = dynamic_cast<const ExpressionStmt*>(&stmt)){
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
        if (const auto* node = dynamic_cast<const AssignmentExpr*>(&expr)) { 
            std::string value = emitExpression(*node->value);
            std::string target = emitExpression(*node->target);
            return "(" + target + " = " + value + ")";
        }
        if (const auto* node = dynamic_cast<const CallExpr*>(&expr)) { 
            if(node->callee == "input"){
                if(!node->arguments.empty()){
                    throw std::runtime_error("input() expects exactly 0 arguments");
                }
                return "gash::input()";
            }
            if(node->callee == "int"){
                if(node->arguments.size() != 1){
                    throw std::runtime_error("int() expects exactly 1 argument");
                }
                std::string argument = emitExpression(*node->arguments[0]);
                return "gash::toInt(" + argument + ")";
            }
            if(node->callee == "float"){
                if(node->arguments.size() != 1){
                    throw std::runtime_error("float() expects exactly 1 argument");
                }
                std::string argument = emitExpression(*node->arguments[0]);
                return "gash::toFloat(" + argument + ")";
            }
            if(node->callee == "string"){
                if(node->arguments.size() != 1){
                    throw std::runtime_error("string() expects exactly 1 argument");
                }
                std::string argument = emitExpression(*node->arguments[0]);
                return "gash::toString(" + argument + ")";
            }
            if(node->callee == "inspect"){
                if(node->arguments.size() != 1){
                    throw std::runtime_error("inspect() expects exactly 1 argument");
                }
                std::string argument = emitExpression(*node->arguments[0]);
                return "gash::inspect(" + argument + ")";
            }
            if(node->callee == "preview"){
                if(node->arguments.size() > 0){
                    throw std::runtime_error("preview() expects exactly 0 argument");
                }
                return "gash::preview()";
            }
            std::string functionName = "gash_" + node->callee;
            std::string result = functionName + "(";
            for (size_t i = 0; i < node->arguments.size(); i++) {
                const auto& argument = node->arguments[i];
                if (i > 0) {
                    result += ", ";
                }
                result += emitExpression(*argument);
            }
            result += ")";
            return result;
        }
        if (const auto* node = dynamic_cast<const ListExpr*>(&expr)) { 
            std::string type = node->elementType;
            std::string cppType = mapType(type);
            std::string size = emitExpression(*node->size);
            std::string result = "gash::List<" + cppType + ">(" + size + ")";
            return result;
        }
        if (const auto* node = dynamic_cast<const IndexExpr*>(&expr)) { 
            std::string collection = emitExpression(*node->collection);
            std::string index = emitExpression(*node->index);
            return "(" + collection + ").get(" + index + ")";
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
        nextRepeatId = 0;
        output.str("");
        output.clear();

        emitLine("#include <iostream>");
        emitLine("#include <string>");
        emitLine("");

        for (const auto& stmt : program.statements) {
            if (dynamic_cast<const FunctionDeclStmt*>(stmt.get())) {
                emitStatement(*stmt);
                emitLine("");
            }
        }

        emitLine("int main(){");
        indentLevel++;

        for (const auto& stmt : program.statements) {
            if (dynamic_cast<const FunctionDeclStmt*>(stmt.get())) {
                continue;
            }
            emitStatement(*stmt);
        }

        emitLine("return 0;");

        indentLevel--;
        emitLine("}");

        return output.str();
    }
    //helper
    std::string CodeGenerator::mapType(const std::string& gashType){
         if (gashType == "int") {
            return "int";
        } else if (gashType == "float") {
            return "double";
        } else if (gashType == "string") {
            return "std::string";
        } else if (gashType == "bool") {
            return "bool";
        } else if (gashType == "char") {
            return "char";
        }

        throw std::runtime_error(
            "Unsupported GASH type: " + gashType
        );
    }
}