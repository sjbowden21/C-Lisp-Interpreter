/*  Lisp Interpreter Phase 1 and 2
    Author: Scott Bowden
    GTID: 903034108
    username: sbowden7

    I worked on this assignment alone. I used course materials
    and AI tools as a reference. I made no internet searches
    relating to logic, code structure, or how to solve
    the exercise.I also specifically requested that no AI code be
    generated at all. All code is my work. */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#define NUMBER "number"
#define SYMBOL "symbol"
#define BOOLEAN "boolean"
#define STRING "string"
#define FUNCTION "function"
#define LIST "list"
#define ERROR "error"
    
// OUTERMOST ENUM FOR TYPES
typedef enum valueType {
    TYPE_Number,
    TYPE_Bool,
    TYPE_Symbol,
    TYPE_List,
    TYPE_Function,
    TYPE_Error,
    TYPE_String
} valueType;

// TAGGED UNION FOR TYPES
typedef struct Value { //access with varName.data.varNameInUnion where varName is of type Value
    valueType typeTag; //tag
    union {
        int64_t numberVal;
        _Bool boolVal;
        char* symbolVal;
        struct List* listVal;
        struct Function* functionVal;
        struct Error* errorVal;
        struct String* stringVal;
    } in; //union
} Value;

//FUNCTION BINDING STRUCTURE
typedef struct Binding {
    char* funcString;
    Value* matchedValue;
} Binding;

//ENVIRONMENT STRUCTURE
typedef struct Env {
    Binding* bindingArr;
    size_t size;
    size_t protectedSize;
    size_t capacity;
    //add parent later for closures
} Env;

//BUILTIN FUNCTION SIGNATURE
typedef Value* (*BuiltInFunc)(Value* argV, Env* environment);

//ENUM TO TAG BUILTINS AND USER DEFINED FUNCTIONS SEPARATELY
typedef enum functionType {
    TYPE_builtin,
    TYPE_lambda //later
} functionType;

//FUNCTION STRUCTURE
typedef struct Function {
    functionType funcTag;
    char* funcName;
    union {
        BuiltInFunc builtIn;
        struct lambda {
            struct List* params;
            //body and closure, later
        } lambda;
    } in;
} Function;

// LIST STRUCTURE
typedef struct List {
    size_t capacity;
    size_t size;
    Value **data;
} List;

//ERROR STRUCTURE
typedef struct Error {
    Value* wrappedValue;
} Error;

typedef struct String {
    size_t length;
    char* value;
} String;

// CREATE BUILT IN FUNCTION FUNCTION
Value* createBuiltIn(BuiltInFunc func, char* name) {
    Value* newBuiltInFunc = malloc(sizeof(Value));
    assert(newBuiltInFunc);
    newBuiltInFunc->typeTag = TYPE_Function;
    newBuiltInFunc->in.functionVal = malloc(sizeof(Function));
    assert(newBuiltInFunc->in.functionVal);
    newBuiltInFunc->in.functionVal->funcTag = TYPE_builtin;
    newBuiltInFunc->in.functionVal->funcName = name;
    newBuiltInFunc->in.functionVal->in.builtIn = func;
    return newBuiltInFunc;
}

// CREATE NUMBER FUNCTION
Value* createNumber(int64_t numberIn) {
    Value* newNumber = malloc(sizeof(Value));
    assert(newNumber);
    newNumber->typeTag = TYPE_Number;
    newNumber->in.numberVal = numberIn;
    return newNumber;
}

// CREATE BOOL FUNCTION
Value* createBool(_Bool boolIn) {
    Value* newBool = malloc(sizeof(Value));
    assert(newBool);
    newBool->typeTag = TYPE_Bool;
    newBool->in.boolVal = boolIn;
    return newBool;
}
// CREATE SYMBOL FUNCTION
Value* createSymbol(char* symbolIn) {
    Value* newSymbol = malloc(sizeof(Value));
    assert(newSymbol);
    newSymbol->typeTag = TYPE_Symbol;
    newSymbol->in.symbolVal = _strdup(symbolIn);
    assert(newSymbol->in.symbolVal);
    return newSymbol;
}

// CREATE LIST FUNCTION (CREATES BLANK LIST)
Value *createList() {
    Value *newList = malloc(sizeof(Value));
    assert(newList);
    newList->typeTag = TYPE_List;
    newList->in.listVal = malloc(sizeof(List));
    assert(newList->in.listVal);
    newList->in.listVal->capacity = 1; //initiates with capacity of 1
    newList->in.listVal->size = 0;
    newList->in.listVal->data = malloc(sizeof(Value*) * newList->in.listVal->capacity);
    assert(newList->in.listVal->data);
    return newList; //returns the pointer, not the structure inside
}
// CREATE ERROR FUNCTION (GENERIC)
Value* createError(Value* valueIn) {
    Value* newError = malloc(sizeof(Value));
    assert(newError);
    newError->typeTag = TYPE_Error;
    newError->in.errorVal = malloc(sizeof(Error));
    assert(newError->in.errorVal);
    newError->in.errorVal->wrappedValue = valueIn;
    return newError;
}
// CREATE STRING FUNCTION
Value* createString(char* stringIn, int64_t stringLength) {
    Value* newString = malloc(sizeof(Value));
    assert(newString);
    newString->typeTag = TYPE_String;
    //edit this, make non-null terminated
    newString->in.stringVal = malloc(sizeof(String));
    assert(newString->in.stringVal);
    if (stringLength == 0) {
        newString->in.stringVal->value = NULL;
    }
    else {
        newString->in.stringVal->value = malloc(sizeof(char));
        char* temp = NULL;
        for (int i = 0; i < stringLength; i++) {
            newString->in.stringVal->value[i] = stringIn[i];
            if (i < stringLength - 1) {
                temp = realloc(newString->in.stringVal->value, (i + 2) * sizeof(char));
                assert(temp);
                newString->in.stringVal->value = temp;
            }
        }
    }
    newString->in.stringVal->length = stringLength;
    return newString;
}

// FREE NUMBER ENTRY FUNCTION
void freeNumber(Value** number) {
    free((*number));
    *number = NULL;
}
// FREE BOOL FUNCTION
void freeBool(Value** bool) {
    free((*bool));
    bool = NULL;
}
// FREE SYMBOL FUNCTION
void freeSymbol(Value** symbol) {
    free((*symbol)->in.symbolVal); //symbolVal was set using strdup so must be free'd
    (*symbol)->in.symbolVal = NULL;
    free((*symbol));
    *symbol = NULL;
}

//FREEVALUE STUB SO IT IS ABOVE FREE ERROR WHICH CALLS IT
void freeValue(Value** input);

// FREE LIST FUNCTION, ONLY FOR A LIST WITH NO NESTED LISTS, I.E. SUBLISTS MUST BE ALREADY FREED, 
// ALSO FIRST ELEMENT IS A GLOBAL FUNCTION AND MUST BE FREE'D LATER
void freeList(Value** oldList) {
    Value* list = *oldList;
    free(list->in.listVal->data);
    list->in.listVal->data = NULL;
    free(list->in.listVal);
    list->in.listVal = NULL;
    free(list);
    *oldList = NULL;
}

// FREE FUNCTION FUNCTION
void freeFunction(Value** function) {
    if ((*function)->in.functionVal->funcTag == TYPE_builtin) {
        free((*function)->in.functionVal);
        (*function)->in.functionVal = NULL;
    }
    else {
        //user defined, free later, more to free
    }
    free((*function));
    *function = NULL;
}

// FREE ERROR FUNCTION
void freeError(Value** error) {
    freeValue(&((*error)->in.errorVal->wrappedValue)); //do not set to null, it is already done in the helper
    free((*error)->in.errorVal);
    (*error)->in.errorVal = NULL;
    free((*error));
    *error = NULL;
}

// FREE STRING FUNCTION
void freeString(Value** stringIn) {
    free((*stringIn)->in.stringVal->value);
    (*stringIn)->in.stringVal->value = NULL;
    free((*stringIn)->in.stringVal);
    (*stringIn)->in.stringVal = NULL; //stringVal was set using strdup so must be free'd
    free(*stringIn);
    *stringIn = NULL;
}

//RECURSIVE FREE ENTIRE S-LIST (RECURSIVE, CALL THIS ON THE OUTER LIST WITH UNKNOWN CONTENTS)
void rFreeList(Value** list) {
    for (int64_t i = 0; i < (*list)->in.listVal->size; i++) {
        if ((*list)->in.listVal->data[i]->typeTag == TYPE_List) {
            rFreeList(&((*list)->in.listVal->data[i]));
        }
        else {
            freeValue(&((*list)->in.listVal->data[i]));
        }
    }
    freeList(list);
}

//FREE ENVIRONMENT (AND BINDING ARRAY WITH THE FUNCTIONS WITHIN)
void freeEnvironment(Env** environment) {
    for (int64_t i = 0; i < (*environment)->size; i++) {
        freeValue(&((*environment)->bindingArr[i].matchedValue)); //this free's the functions
    }
    free((*environment)->bindingArr);
    free(*environment);
    *environment = NULL;
}

/*FREE VALUE, THIS FREES A VALUE* OBJECT AND IS THE OUTERMOST FUNCTION, IT DIRECTS TO APPLICABLE HELPERS.
WRAPPED VALUES WITHIN THE ERROR TYPE ARE ALSO PASSED INTO THIS FUNCTION
*/ 
void freeValue(Value** input) {
    switch ((*input)->typeTag) {
    case TYPE_Number:
        freeNumber(input);
        break;
    case TYPE_Bool:
        freeBool(input);
        break;
    case TYPE_Symbol:
        freeSymbol(input);
        break;
    case TYPE_List:
        rFreeList(input);
        break;
    case TYPE_Function:
        freeFunction(input);
        break;
    case TYPE_Error:
        freeError(input);
        break;
    case TYPE_String:
        freeString(input);
        break;
    }
}

Value* lookupBuiltinIndex(Env* environment, Value* symbolIn) {
    char* lookupVal = symbolIn->in.symbolVal;
    _Bool found = 0;
    int64_t index = 0;
    while (!found && index < environment->size) {
        if (strcmp(lookupVal, environment->bindingArr[index].funcString) == 0) {
            found = 1;
        }
        index++;
    }
    if (found) {
        return --index;
    }
    else {
        return -1;
    }

}

//LOOK UP SYMBOL IN ENVIRONMENT (THE SYMBOL POINTER IS WHAT IS PASSED)
Value* lookupBuiltin(Env* environment, Value* symbolIn) {
    int64_t index = lookupBuiltinIndex(environment, symbolIn);
    if (index >= 0) {
        return environment->bindingArr[index].matchedValue;
    }
    else {
        return NULL;
    }
}


//LIST ADD TO LIST FUNCTION
void addToList(Value* currentList, Value* valueToAdd) {
    if (currentList->in.listVal->size == currentList->in.listVal->capacity) {
        currentList->in.listVal->capacity *= 2;
        Value** temp = realloc(currentList->in.listVal->data, currentList->in.listVal->capacity * sizeof(Value*)); //\
           // * currentList->in.listVal->capacity);
        assert(temp);
        currentList->in.listVal->data = temp;
    }
    currentList->in.listVal->data[currentList->in.listVal->size] = valueToAdd;
    currentList->in.listVal->size++;
}

//STUB FOR COPY VALUE SO RECURSIVE COPY CAN SEE THE SIGNATURE
Value* copyValue(Value* input);

//RECURSIVE COPY LIST
Value* rCopyList(Value* inputList) {
    Value* outputList = createList();
    for (int64_t i = 0; i < inputList->in.listVal->size; i++) {
        Value* output = NULL;
        if (inputList->in.listVal->data[i]->typeTag == TYPE_List) {
            output = rCopyList(inputList->in.listVal->data[i]);
        }
        else {
            output = copyValue(inputList->in.listVal->data[i]);
        }
        addToList(outputList, output);
    }
    return outputList;
}

// CREATE A DEEP COPY OF A VALUE
Value* copyValue(Value* input) {
    Value* output = malloc(sizeof(Value));
    assert(output);
    output->typeTag = input->typeTag;
    switch (input->typeTag) {
    case TYPE_Number:
        output->in.numberVal = input->in.numberVal;
        break;
    case TYPE_Bool:
        output->in.boolVal = input->in.boolVal;
        break;
    case TYPE_Symbol:
        output->in.symbolVal = _strdup(input->in.symbolVal);
        break;
    case TYPE_String:
        output->in.stringVal = malloc(sizeof(String));
        assert(output->in.stringVal);
        output->in.stringVal->value = malloc(sizeof(char));
        assert(output->in.stringVal->value);
        output->in.stringVal->length = input->in.stringVal->length;
        char* temp = NULL;

        for (int64_t i = 0; i < input->in.stringVal->length; i++) {
            output->in.stringVal->value[i] = input->in.stringVal->value[i];
            if (i < input->in.stringVal->length) {
                temp = realloc(output->in.stringVal->value, (i + 2) * sizeof(char));
                assert(temp);
                output->in.stringVal->value = temp;
            }
        }
        break;
    case TYPE_Function:
        output->in.functionVal = malloc(sizeof(Function));
        assert(output->in.functionVal);
        output->in.functionVal->funcTag = input->in.functionVal->funcTag;
        output->in.functionVal->funcName = input->in.functionVal->funcName;
        if (input->in.functionVal->funcTag == TYPE_builtin) {
            output->in.functionVal->in.builtIn = input->in.functionVal->in.builtIn;
        }
        else {
            //lambda, do later
        }
        return output;
        break;
    case TYPE_List:
        output = rCopyList(input);
        break;
    }
    //if needed, list, error, function will be added later
    return output;
}

//INVALID TOKEN ERROR CREATOR
Value* invalidTokenError(char* badString) {
    Value* badTokenList = createList();
    const char description[] = "invalid-token";
    Value* descSymb = createSymbol(description);
    int64_t location = strcspn(badString, "\0");
    Value* badStr = createString(badString, location);
    addToList(badTokenList, descSymb);
    addToList(badTokenList, badStr);
    Value* errorOut = createError(badTokenList);
    return errorOut;
}

//DIVIDE BY ZERO ERROR CREATOR
Value* divideByZeroError(void) {
    const char description[] = "division-by-zero";
    Value* descSymb = createSymbol(description);
    Value* errorOut = createError(descSymb);
    return errorOut;
}

//UNBOUND SYMBOL ERROR CREATOR
Value* unboundSymbol(char* unboundSymbStr) {
    const char description[] = "unbound-symbol";
    Value* descSymb = createSymbol(description);
    Value* unboundSymb = createSymbol(unboundSymbStr);
    Value* badList = createList();
    addToList(badList, descSymb);
    addToList(badList, unboundSymb);
    Value* errorOut = createError(badList);
    return errorOut;
}

//ARITY ERROR ERROR CREATOR
Value* arityError(char* badFunctionSymbStr, char* cmp, int64_t expected, int64_t received) {
    const char description[] = "arity-error";
    Value* descSymb = createSymbol(description);
    Value* badFunctionSymb = createSymbol(badFunctionSymbStr);
    Value* compareSymb = createSymbol(cmp);
    Value* expectedNum = createNumber(expected);
    Value* compareList = createList();
    addToList(compareList, compareSymb);
    addToList(compareList, expectedNum);
    Value* receivedNum = createNumber(received);
    Value* badList = createList();
    addToList(badList, descSymb);
    addToList(badList, badFunctionSymb);
    addToList(badList, compareList);
    addToList(badList, receivedNum);
    Value* errorOut = createError(badList);
    return errorOut;
}
Value* typeError(char* badFunctionSymbStr, int64_t position, char* expectedType, Value* receivedType) {
    const char description[] = "type-error";
    Value* descSymb = createSymbol(description);
    Value* badFunctionSymb = createSymbol(badFunctionSymbStr);
    Value* posNum = createNumber(position);
    Value* expectedTypeSymb = createSymbol(expectedType);
    Value* badList = createList();
    addToList(badList, descSymb);
    addToList(badList, badFunctionSymb);
    addToList(badList, posNum);
    addToList(badList, expectedTypeSymb);
    addToList(badList, receivedType);
    Value* errorOut = createError(badList);
    return errorOut;
}
Value* valueError(char* badFunctionSymbStr, Value* badInput) { 
    //the bad input must be passed in as Value*, the caller must create it because the typeTag can vary
    const char description[] = "value-error";
    Value* descSymbol = createSymbol(description);
    Value* badFunctionSymb = createSymbol(badFunctionSymbStr);
    Value* badList = createList();
    addToList(badList, descSymbol);
    addToList(badList, badFunctionSymb);
    addToList(badList, badInput);
    Value* errorOut = createError(badList);
    return errorOut;
}

Value* headError(void) {
    const char description[] = "inapplicable-head";
    Value* descSymb = createSymbol(description);
    Value* errorOut = createError(descSymb);
    return errorOut;
}

Value* protectedSymbolError(char* badSymbStr) {
    const char description[] = "protected-symbol";
    Value* descSymb = createSymbol(description);
    Value* badSymb = createSymbol(badSymbStr);
    Value* badList = createList();
    addToList(badList, descSymb);
    addToList(badList, badSymb);
    Value* errorOut = createError(badList);
    return errorOut;
}

Value* incompleteParseError(char* badString) {
    const char description[] = "incomplete-parse";
    Value* descSymb = createSymbol(description);
    int64_t location = strcspn(badString, "\0");
    Value* badStr = createString(badString, location);
    Value* badList = createList();
    addToList(badList, descSymb);
    addToList(badList, badStr);
    Value* errorOut = createError(badList);
    return errorOut;
}

// PLUS FUNCTION
Value* builtinAdd(Value* argList, Env* environment) {
    Value* output = NULL;
    int64_t count = 1; //start at 1 because index 0 in the list is the function value
    int64_t runningSum = 0;
    while (count < argList->in.listVal->size) {
        if (argList->in.listVal->data[count]->typeTag != TYPE_Number) {
            output = typeError(argList->in.listVal->data[0]->in.functionVal->funcName, count, NUMBER,
                copyValue(argList->in.listVal->data[count]));
            return output;
        }
        else {
            runningSum += argList->in.listVal->data[count]->in.numberVal;
        }
        count++;
    }
    output = createNumber(runningSum);
    return output;
}

// SUBTRACT FUNCTION
Value* builtinSubtract(Value* argList, Env* environment) {
    Value* output = NULL;
    if (argList->in.listVal->size == 1) { //only subraction sign present
        output = arityError(argList->in.listVal->data[0]->in.functionVal->funcName, ">=", 1, argList->in.listVal->size - 1);
        return output;
    }
    else {
        int64_t count = 1;
        int64_t runningDiff = 0;
        if (argList->in.listVal->size == 2) {
            runningDiff = argList->in.listVal->data[count]->in.numberVal * -1;
        }
        else {
            runningDiff = argList->in.listVal->data[count]->in.numberVal;
        }
        count++;
        while (count < argList->in.listVal->size) {
            if (argList->in.listVal->data[count]->typeTag != TYPE_Number) {
                output = typeError(argList->in.listVal->data[0]->in.functionVal->funcName, count, NUMBER,
                    copyValue(argList->in.listVal->data[count]));
                return output;
            }
            else {
                runningDiff -= argList->in.listVal->data[count]->in.numberVal;
            }
            count++;
        }
        output = createNumber(runningDiff);
        return output;
    }
}

// MULTIPLY FUNCTION
Value* builtinMultiply(Value* argList, Env* environment) {
    Value* output = NULL;
    int64_t count = 1;
    int64_t runningProduct = 1;
    while (count < argList->in.listVal->size) {
        if (argList->in.listVal->data[count]->typeTag != TYPE_Number) {
            output = typeError(argList->in.listVal->data[0]->in.functionVal->funcName, count, NUMBER,
                copyValue(argList->in.listVal->data[count]));
            return output;
        }
        else {
            runningProduct *= argList->in.listVal->data[count]->in.numberVal;
        }
        count++;
    }
    output = createNumber(runningProduct);
    return output;
}

// DIVIDE FUNCTION
Value* builtinDivide(Value* argList, Env* environment) {
    Value* output = NULL;
    if (argList->in.listVal->size < 3) {
        return arityError(argList->in.listVal->data[0]->in.functionVal->funcName, ">=", 2, argList->in.listVal->size - 1);
    }
    else {
        int64_t runningQuotient = NULL;
        int64_t count = 1;
        if (argList->in.listVal->data[count]->typeTag != TYPE_Number) {
            return typeError(argList->in.listVal->data[0]->in.functionVal->funcName, count,
                NUMBER, copyValue(argList->in.listVal->data[count]));
        }
        else {
            runningQuotient = argList->in.listVal->data[count]->in.numberVal;
            while (count < argList->in.listVal->size - 1) {
                if (argList->in.listVal->data[count + 1]->typeTag != TYPE_Number) {
                    return typeError(argList->in.listVal->data[0]->in.functionVal->funcName, count + 1,
                        NUMBER, copyValue(argList->in.listVal->data[count + 1]));
                }
                else {
                    if (argList->in.listVal->data[count + 1]->in.numberVal == 0) {
                        return divideByZeroError();
                    }
                    else {
                        if (runningQuotient % argList->in.listVal->data[count + 1]->in.numberVal != 0 \
                                && runningQuotient * argList->in.listVal->data[count + 1]->in.numberVal < 0) {
                            runningQuotient /= argList->in.listVal->data[count + 1]->in.numberVal;
                            runningQuotient -= 1;
                        }
                        else {
                            runningQuotient /= argList->in.listVal->data[count + 1]->in.numberVal;
                        }
                    }
                }
                count++;
            }
            output = createNumber(runningQuotient);
            return output;
        }
    }
}

// EQUALS FUNCTION
Value* builtinEquals(Value* argList, Env* environment) {
    _Bool resultBool = NULL;
    if (argList->in.listVal->size <= 2) {
        resultBool = 1;
        return createBool(resultBool);
    }
    else { //first make sure all the types are the same, to avoid unnecessary comparisons
        int64_t count = 2;
        while (count < argList->in.listVal->size) {
            if (argList->in.listVal->data[1]->typeTag != argList->in.listVal->data[count]->typeTag) {
                resultBool = 0;
                return createBool(resultBool);
            }
            count++;
        }
        count = 1; //all types match if this line is reached
        Value* current = argList->in.listVal->data[count];
        while (count < argList->in.listVal->size - 1) {
            switch (argList->in.listVal->data[count]->typeTag) {
            case TYPE_Number:
                resultBool = argList->in.listVal->data[count]->in.numberVal == 
                    argList->in.listVal->data[count + 1]->in.numberVal;
                if (!resultBool) {
                    return createBool(resultBool);
                }
                break;
            case TYPE_Symbol:
                resultBool = strcmp(argList->in.listVal->data[count]->in.symbolVal,
                    argList->in.listVal->data[count + 1]->in.symbolVal) == 0;
                if (!resultBool) {
                    return createBool(resultBool);
                }
                break;
            case TYPE_Bool:
                resultBool = argList->in.listVal->data[count]->in.boolVal ==
                    argList->in.listVal->data[count + 1]->in.boolVal;
                if (!resultBool) {
                    return createBool(resultBool);
                }
                break;
            case TYPE_String:
                break;
            case TYPE_Function:
                break;
            case TYPE_List:
                break;
            case TYPE_Error:
                break;
            }
            count++;
        }
        return createBool(resultBool);
    }
}

// QUIT FUNCTION
Value* builtinQuit(Value* argList, Env* environment) {

    if (argList->in.listVal->size > 1) {
        return arityError(argList->in.listVal->data[0]->in.functionVal->funcName, "=", 0, argList->in.listVal->size - 1);
    }
    else {
        return copyValue(argList->in.listVal->data[0]);
    }
}
// <, <=, >, AMD >= FUNCTIONS BELOW ALMOST ENTIRELY SHARE CODE. I WILL REFACTOR IF TIME BUT FOR NOW WILL GET IT WORKING

// LESS THAN FUNCTION
Value* builtinLessThan(Value* argList, Env* environment) {
    _Bool resultBool = NULL;
    if (argList->in.listVal->size <= 2) {
        resultBool = 1;
        return createBool(resultBool);
    }
    else {
        if (argList->in.listVal->data[1]->typeTag != TYPE_Number) {
            return typeError(argList->in.listVal->data[0]->in.functionVal->funcName, 1,
                NUMBER, copyValue(argList->in.listVal->data[1]));
        }
        for (int64_t i = 2; i < argList->in.listVal->size; i++) {
            if (argList->in.listVal->data[i]->typeTag != TYPE_Number) {
                return typeError(argList->in.listVal->data[0]->in.functionVal->funcName, i,
                    NUMBER, copyValue(argList->in.listVal->data[i]));
            }
            else { //can safey check numberVal, these are numbers
                if (argList->in.listVal->data[i-1]->in.numberVal >= argList->in.listVal->data[i]->in.numberVal) {
                    resultBool = 0;
                    return createBool(resultBool);
                }
            }
        }
        resultBool = 1;
        return createBool(resultBool);
    }
}

// LESS THAN OR EQUAL FUNCTION
Value* builtinLessThanOrEqual(Value* argList, Env* environment) {
    _Bool resultBool = NULL;
    if (argList->in.listVal->size <= 2) {
        resultBool = 1;
        return createBool(resultBool);
    }
    else {
        if (argList->in.listVal->data[1]->typeTag != TYPE_Number) {
            return typeError(argList->in.listVal->data[0]->in.functionVal->funcName, 1,
                NUMBER, copyValue(argList->in.listVal->data[1]));
        }
        for (int64_t i = 2; i < argList->in.listVal->size; i++) {
            if (argList->in.listVal->data[i]->typeTag != TYPE_Number) {
                return typeError(argList->in.listVal->data[0]->in.functionVal->funcName, i,
                    NUMBER, copyValue(argList->in.listVal->data[i]));
            }
            else { //can safey check numberVal, these are numbers
                if (argList->in.listVal->data[i-1]->in.numberVal > argList->in.listVal->data[i]->in.numberVal) {
                    resultBool = 0;
                    return createBool(resultBool);
                }
            }
        }
        resultBool = 1;
        return createBool(resultBool);
    }
}

//GREATER THAN FUNCTION
Value* builtinGreaterThan(Value* argList, Env* environment) {
    _Bool resultBool = NULL;
    if (argList->in.listVal->size <= 2) {
        resultBool = 1;
        return createBool(resultBool);
    }
    else {
        if (argList->in.listVal->data[1]->typeTag != TYPE_Number) {
            return typeError(argList->in.listVal->data[0]->in.functionVal->funcName, 1,
                NUMBER, copyValue(argList->in.listVal->data[1]));
        }
        for (int64_t i = 2; i < argList->in.listVal->size; i++) {
            if (argList->in.listVal->data[i]->typeTag != TYPE_Number) {
                return typeError(argList->in.listVal->data[0]->in.functionVal->funcName, i,
                    NUMBER, copyValue(argList->in.listVal->data[i]));
            }
            else { //can safey check numberVal, these are numbers
                if (argList->in.listVal->data[i - 1]->in.numberVal <= argList->in.listVal->data[i]->in.numberVal) {
                    resultBool = 0;
                    return createBool(resultBool);
                }
            }
        }
        resultBool = 1;
        return createBool(resultBool);
    }
}

//GREATER THAN OR EQUAL FUNCTION
Value* builtinGreaterThanOrEqual(Value* argList, Env* environment) {
    _Bool resultBool = NULL;
    if (argList->in.listVal->size <= 2) {
        resultBool = 1;
        return createBool(resultBool);
    }
    else {
        if (argList->in.listVal->data[1]->typeTag != TYPE_Number) {
            return typeError(argList->in.listVal->data[0]->in.functionVal->funcName, 1,
                NUMBER, copyValue(argList->in.listVal->data[1]));
        }
        for (int64_t i = 2; i < argList->in.listVal->size; i++) {
            if (argList->in.listVal->data[i]->typeTag != TYPE_Number) {
                return typeError(argList->in.listVal->data[0]->in.functionVal->funcName, i,
                    NUMBER, copyValue(argList->in.listVal->data[i]));
            }
            else { //can safey check numberVal, these are numbers
                if (argList->in.listVal->data[i - 1]->in.numberVal < argList->in.listVal->data[i]->in.numberVal) {
                    resultBool = 0;
                    return createBool(resultBool);
                }
            }
        }
        resultBool = 1;
        return createBool(resultBool);
    }
}

//NOT EQUAL FUNCTION
Value* builtinNotEqual(Value* argList, Env* environment) {
    Value* equalsOutput = builtinEquals(argList, environment);
    equalsOutput->in.boolVal =  !equalsOutput->in.boolVal;
    return equalsOutput;
}

Value* builtinNot(Value* argList, Env* environment) {
    _Bool resultBool = NULL;
    if (argList->in.listVal->size != 2) {
        return arityError(argList->in.listVal->data[0]->in.functionVal->funcName, "=", 1, argList->in.listVal->size - 1);
    }
    else {
        if (argList->in.listVal->data[1]->typeTag == TYPE_Bool) {
            if (argList->in.listVal->data[1]->in.boolVal == 0) {
                resultBool = 1;
                return createBool(resultBool);
            }
        }
        resultBool = 0;
        return createBool(resultBool);
    }
}

//ADD A BINDING TO THE ENVIRONMENT
void addToEnvironment(Env* environment, char* stringDesc, Value* addedValue, _Bool protected) {
    if (environment->size == environment->capacity) {
        environment->capacity *= 2;
        Binding* temp = realloc(environment->bindingArr, environment->capacity * sizeof(Binding));
        assert(temp);
        environment->bindingArr = temp;
    }
    environment->bindingArr[environment->size].funcString = stringDesc;
    environment->bindingArr[environment->size].matchedValue = addedValue;
    environment->size++;
    if (protected) {
        environment->protectedSize++;
    }
}

Env* initEnvironment(void) {
    Value* plusFunc = createBuiltIn(builtinAdd, "+");
    Value* minusFunc = createBuiltIn(builtinSubtract, "-");
    Value* multiplyFunc = createBuiltIn(builtinMultiply, "*");
    Value* divideFunc = createBuiltIn(builtinDivide, "/");
    Value* equalsFunc = createBuiltIn(builtinEquals, "=");
    Value* quitFunc = createBuiltIn(builtinQuit, "quit");
    Value* lessThanFunc = createBuiltIn(builtinLessThan, "<");
    Value* lessThanOrEqualFunc = createBuiltIn(builtinLessThanOrEqual, "<=");
    Value* greaterThanFunc = createBuiltIn(builtinGreaterThan, ">");
    Value* greaterThanOrEqualFunc = createBuiltIn(builtinGreaterThanOrEqual, ">=");
    Value* notEqualFunc = createBuiltIn(builtinNotEqual, "!=");
    Value* notFunc = createBuiltIn(builtinNot, "not");

    Env* environment = malloc(sizeof(Env));
    assert(environment);
    environment->size = 0;
    environment->protectedSize = 0;
    environment->capacity = 10; //initial capacity of 10
    environment->bindingArr = malloc(environment->capacity * sizeof(Binding));
    assert(environment->bindingArr);

    addToEnvironment(environment, "+", plusFunc, 1);
    addToEnvironment(environment, "-", minusFunc, 1);
    addToEnvironment(environment, "*", multiplyFunc, 1);
    addToEnvironment(environment, "/", divideFunc, 1);
    addToEnvironment(environment, "=", equalsFunc, 1);
    addToEnvironment(environment, "quit", quitFunc, 1);
    addToEnvironment(environment, "<", lessThanFunc, 1);
    addToEnvironment(environment, "<=", lessThanOrEqualFunc, 1);
    addToEnvironment(environment, ">", greaterThanFunc, 1);
    addToEnvironment(environment, ">=", greaterThanOrEqualFunc, 1);
    addToEnvironment(environment, "!=", notEqualFunc, 1);
    addToEnvironment(environment, "not", notFunc, 1);
    return environment;
}

// bad token creator stub for compiler
char* badTokenCreator(int tokenIndex, const char* userInput, int* currentIndex);

// VALIDATE NUMBER FUNCTION
Value* numberValidator(const char* userInput, int* currentIndex, Value* currentList, Value* currentOutput, _Bool isPositive, Value** quoteList, _Bool isQuoteVal) {
    _Bool errorFound = 0;
    _Bool spaceFound = isspace(userInput[(*currentIndex)]);
    char currentChar = userInput[(*currentIndex)];
    int asNumber = (int)userInput[*currentIndex] - 48;
    int64_t runningNumber = 0;
    size_t count = 1;
    char* runningString = malloc(count * sizeof(char));
    assert(runningString);
    char* temp = NULL;
    while (!spaceFound && currentChar != '\0'&& currentChar != '(' && currentChar != ')') {
        runningString[count - 1] = currentChar;
        if (asNumber <= 9 && asNumber >= 0) {
            runningNumber = runningNumber * 10 + asNumber;
        }
        else {
            errorFound = 1; //not a number, not a space because loop never runs when current char is a space
        }
        (*currentIndex)++;
        count++;
        temp = realloc(runningString, count * sizeof(char));
        assert(temp);
        runningString = temp;
        spaceFound = isspace(userInput[*currentIndex]); //*if no error, currentIndex ends where space is located
        currentChar = userInput[(*currentIndex)];
        asNumber = (int)userInput[*currentIndex] - 48;
    }
    runningString[count - 1] = '\0'; //add null terminator at end   
    if ((currentChar == ')' || currentChar == '(') && !isQuoteVal) {
        (*currentIndex)--; //must be decremented so parser logic can be applied to the ')' or '('
    }
    if (errorFound) {
        //error is a symbol (faulty number), it's an invalid token error
        Value* errorOut = invalidTokenError(runningString);
        return errorOut;
    }
    else {
        if (!isPositive) {
            runningNumber *= -1;
        }
        Value* numberToAdd = createNumber(runningNumber);
        if (currentList != NULL) {
            addToList(currentList, numberToAdd); //within open (
            return numberToAdd;
        }
        else if (currentOutput != NULL) {
            if ((*quoteList) != NULL) {
                Value* quoteListTail = currentOutput;
                if (quoteListTail->typeTag == TYPE_List) {
                    addToList(quoteListTail, numberToAdd);
                }
                Value* outputList = copyValue(*quoteList);
                freeValue(&(*quoteList));
                //quoteList = NULL;
                quoteListTail = NULL;
                currentOutput = NULL;
                return outputList;
            }
            else {
                // error, two values but no list, do later, invalid token
                char* badToken = badTokenCreator(0, userInput, currentIndex);
                return invalidTokenError(badToken);
            }
        }
        else {
            return numberToAdd;
        }
    }
}

// IS CHARACTER A VALID SYMBOL FUNCTION
_Bool isSymbol(char in) {
    return in == '*' || in == '/' || in == '%' || in == '=' || in == '<' ||
        in == '>' || in == '!' || in == ':' || in == '+' || in == '-';
}

// VALIDATE SYMBOL FUNCTION
Value* symbolValidator(const char* userInput, int* currentIndex, Value* currentList, _Bool isAllSymbol, _Bool isSpecial, Value* currentOutput, Value** quoteList, _Bool isQuoteVal) {
    _Bool errorFound = 0;
    _Bool spaceFound = isspace(userInput[*currentIndex]);
    _Bool letterBad = 0;
    _Bool numberBad = 0;
    char currentChar = userInput[*currentIndex];
    int asciiValue = (int)userInput[*currentIndex];
    size_t count = 1;
    char* runningSymbol = malloc(count * sizeof(char));
    assert(runningSymbol);
    char* temp = NULL;
    while (!spaceFound && currentChar != '\0' && currentChar != '(' && currentChar != ')') {
        runningSymbol[count - 1] = currentChar;
        if (isAllSymbol) {
            if (!isSymbol(currentChar)) {
                errorFound = 1;
            }
        }
        else {
            letterBad = (!(asciiValue >= 65 && asciiValue <= 90) && !(asciiValue >= 97 && asciiValue <= 122));
            numberBad = !(asciiValue >= 48 && asciiValue <= 57);
            if (!isSymbol(currentChar) && letterBad && numberBad) {
                errorFound = 1;
            }
        }
        (*currentIndex)++;
        count++;
        temp = realloc(runningSymbol, count * sizeof(char));
        assert(temp);
        runningSymbol = temp;
        spaceFound = isspace(userInput[*currentIndex]);
        currentChar = userInput[*currentIndex];
        asciiValue = (int)userInput[*currentIndex];
    }
    runningSymbol[count - 1] = '\0'; /*add \0 terminator at end*/
    if ((currentChar == ')' || currentChar == '(') && !isQuoteVal) {
        (*currentIndex)--; //must be decremented so parser logic can be applied to the ')' or '('
    }
    if (errorFound) {
        Value* errorOut = invalidTokenError(runningSymbol);
        return errorOut;
    }
    else {
        Value* newSymbol = createSymbol(runningSymbol);
        if (currentList != NULL) {
            addToList(currentList, newSymbol); //within open (
            return newSymbol;
        }
        else if (currentOutput != NULL) {
            if ((*quoteList) != NULL) {
                Value* quoteListTail = currentOutput;
                if (quoteListTail->typeTag == TYPE_List) {
                    addToList(quoteListTail, newSymbol);
                }
                Value* outputList = copyValue(*quoteList);
                freeValue(&(*quoteList));
                //quoteList = NULL;
                quoteListTail = NULL;
                currentOutput = NULL;
                return outputList;
            }
            else {
                // error, two values but no list, do later, invalid token
                char* badToken = badTokenCreator(0, userInput, currentIndex);
                return invalidTokenError(badToken);
            }
        }
        else {
            return newSymbol;
        }
    }
}

//STRING BUILDER HELPER
Value* stringBuilder(const char* userInput, int* currentIndex, Value* currentList, Value* currentOutput, Value** quoteList, _Bool isQuoteVal) {
    size_t outputLength = 1;
    char* runningString = malloc(outputLength * sizeof(char));
    assert(runningString);
    runningString[0] = userInput[*currentIndex]; //always a "
    char* temp = NULL;
    _Bool errorFound = 0;
    _Bool closeQuoteFound = 0;
    while (!closeQuoteFound && (*currentIndex) <= strlen(userInput)) { //second condition includes \0 at end
        (*currentIndex)++;
        if ((userInput[*currentIndex] == '\0'  && (*currentIndex) == strlen(userInput))) {
                //|| isspace(userInput[*currentIndex])) {
            errorFound = 1;
        }
        else if (userInput[*currentIndex] == '\"') {
            closeQuoteFound = 1;
        }
        if (userInput[*currentIndex] != '\"') {
            outputLength++;
            temp = realloc(runningString, outputLength * sizeof(char));
            assert(temp);
            runningString = temp;
            if (userInput[*currentIndex] == '\\') {
                if (userInput[*currentIndex + 1] == '0' || userInput[*currentIndex + 1] == 'n'
                    || userInput[*currentIndex + 1] == '\"' || userInput[*currentIndex + 1] == '\\') {
                    if (userInput[*currentIndex + 1] == '0') {
                        runningString[outputLength - 1] = '\0';
                    }
                    else if (userInput[*currentIndex + 1] == 'n') {
                        runningString[outputLength - 1] = '\n';
                    }
                    else if (userInput[*currentIndex + 1] == '\"') {
                        runningString[outputLength - 1] = '\"';
                    }
                    else {
                        runningString[outputLength - 1] = '\\';
                    }
                    (*currentIndex)++;
                }
                else if (userInput[*currentIndex + 1] == 'x' && *currentIndex + 3 < strlen(userInput)) {
                    if ((((int)userInput[*currentIndex + 2] >= 48 && (int)userInput[*currentIndex + 2] <= 57) ||
                            ((int)userInput[*currentIndex + 2] >= 65 && (int)userInput[*currentIndex + 2] <= 70) ||
                            ((int)userInput[*currentIndex + 2] >= 97 && (int)userInput[*currentIndex + 2] <= 102)) &&
                            (((int)userInput[*currentIndex + 3] >= 48 && (int)userInput[*currentIndex + 3] <= 57) ||
                            ((int)userInput[*currentIndex + 3] >= 65 && (int)userInput[*currentIndex + 3] <= 70) ||
                            ((int)userInput[*currentIndex + 3] >= 97 && (int)userInput[*currentIndex + 3] <= 102))) {
                        char hexString[3] = { userInput[*currentIndex + 2], userInput[*currentIndex + 3], '\0'};
                        unsigned int ascii = strtol(hexString, NULL, 16);
                        char char2Add = (char)ascii;
                        runningString[outputLength - 1] = char2Add;
                        (*currentIndex) += 3;
                        //print / in the print function
                    }
                }
                else {
                    runningString[outputLength - 1] = '\\';
                }
            }
            else {
                runningString[outputLength - 1] = userInput[*currentIndex];
            }
        }
    }
    if (errorFound) {
        return incompleteParseError(runningString);
    }
    else {
        //good string, take off the quotes, createString handles empty strings
        if (outputLength > 1) {
            char* newStringCopy = malloc(outputLength - 1 * sizeof(char));
            assert(newStringCopy);
            for (int64_t i = 0; i < outputLength - 1; i++) {
                newStringCopy[i] = runningString[i + 1];
            }
            free(runningString);
            runningString = newStringCopy;
            temp = realloc(runningString, outputLength * sizeof(char));
            assert(temp);
            runningString = temp;
        }
        runningString[outputLength - 1] = '\0';
        Value* newString = createString(runningString, outputLength - 1);
        if (currentList != NULL) {
            addToList(currentList, newString); //within open (
            return newString;
        }
        else if (currentOutput != NULL) {
            if ((*quoteList) != NULL) {
                Value* quoteListTail = currentOutput;
                if (quoteListTail->typeTag == TYPE_List) {
                    addToList(quoteListTail, newString);
                }
                Value* outputList = copyValue(*quoteList);
                freeValue(&(*quoteList));
                //quoteList = NULL;
                quoteListTail = NULL;
                currentOutput = NULL;
                return outputList;
            }
            else {
                // error, two values but no list, do later, invalid token
                char* badToken = badTokenCreator(0, userInput, currentIndex);
                return invalidTokenError(badToken);
            }
        }
        else {
            return newString;
        }
    }
}

//BAD TOKEN CREATOR HELPER
char* badTokenCreator(int tokenIndex, const char* userInput, int* currentIndex) {
    size_t outputLength = 1;
    char* output = malloc(outputLength * sizeof(char));
    assert(output);
    char* temp = NULL;
    while (userInput[tokenIndex] != '\0' &&
        (!isspace(userInput[tokenIndex]) || *currentIndex >= tokenIndex)) {
        output[outputLength - 1] = userInput[tokenIndex];
        tokenIndex++;
        outputLength++;
        temp = realloc(output, outputLength * sizeof(char));
        assert(temp);
        output = temp;
    }
    output[outputLength - 1] = '\0'; //last index already allocated on final loop iteration, this makes output a C string
    return output;
}

Value* parenManager(char* userInput, int* currentIndex);

Value* rQuoteHelper(char* userInput, int* currentIndex, Value* currentList, int startIndex, _Bool isOuter) {
    Value* output = NULL;
    _Bool runLoop = 1;
    while (runLoop/*userInput[*currentIndex] == '\'' || userInput[*currentIndex] == '\0' || isspace(userInput[*currentIndex])*/) {
        if (userInput[*currentIndex] == '\'') {
            Value* quoteList = createList();
            Value* quoteSymb = createSymbol("quote");
            addToList(quoteList, quoteSymb);
            addToList(currentList, quoteList);
            (*currentIndex)++;
            output = rQuoteHelper(userInput, currentIndex, quoteList, startIndex, isOuter);
            runLoop = 0;
        }
        else if (userInput[*currentIndex] == '\0' || userInput[*currentIndex] == ')') { //covers ' followed by end of input, and ' followed by ) which doesn't make sense
            size_t sizeBadBuffer = *currentIndex - startIndex + 1;
            char* badBuffer = malloc(sizeBadBuffer * sizeof(char));
            assert(badBuffer);
            int index = 0;
            while (userInput[startIndex] != '\0') {
                badBuffer[index] = userInput[startIndex];
                startIndex++;
                index++;
            }
            if (userInput[*currentIndex] == '\0') {
                return incompleteParseError(badBuffer);
            }
            else {
                return invalidTokenError(badBuffer);
            }
        }
        else if (!isspace(userInput[*currentIndex])) {
            return currentList;
        }
        else {
            (*currentIndex)++;
        }
    }
    return output;
}

//OUTER AND NESTED PARSER
Value* parser(const char* userInput, int* currentIndex, Value* currentList, _Bool isOuter, _Bool isQuoteVal) {
    Value* output = NULL;
    char* badToken = NULL;
    _Bool errorFound = 0;
    _Bool isAllSymb = 0;
    _Bool isSpecial = 0;
    _Bool isPositive = 0;
    int64_t startIndexCopy = *currentIndex;
    int64_t inputLength = strlen(userInput);
    int64_t count = 1;
    char current;
    Value* quoteList = NULL; // used in ( and ' sections
    Value* quoteListTail = NULL;
    while (!errorFound && *currentIndex < inputLength) { /*second condition omits '\0' at end*/
        current = userInput[*currentIndex];
        int asciiValue = (int)userInput[*currentIndex];
        //for all conditionals, checking next index is ok, because no case is the value at the newline (ending) location
        if (asciiValue >= 48 && asciiValue <= 57) {
            isPositive = 1;
            output = numberValidator(userInput, currentIndex, currentList, output, isPositive, &quoteList, isQuoteVal);
            if (output->typeTag == TYPE_Error || (!isOuter && isQuoteVal)) {
                return output;
            }
        }
        else if ((asciiValue >= 65 && asciiValue <= 90) || (asciiValue >= 97 && asciiValue <= 122)) {
            //letter, but in this format, only possiblity is symbol
            isAllSymb = 0;
            output = symbolValidator(userInput, currentIndex, currentList, isAllSymb, isSpecial, output, &quoteList, isQuoteVal);
            if (output->typeTag == TYPE_Error || (!isOuter && isQuoteVal)) {
                return output;
            }
        }
        else if (current == '+' || current == '-') {
            //symbol or number
            if (isspace(userInput[*currentIndex + 1]) || userInput[*currentIndex + 1] == ')'
                || userInput[*currentIndex + 1] == '(' || userInput[*currentIndex + 1] == '\0') {
                //made symbol with + or -, add to currentList (may be a list at any level of s-list, address passed in to function as pointer)
                char plusMinusStr[2] = { current, '\0' };
                Value* plusMinusSymb = createSymbol(plusMinusStr);
                if (currentList != NULL) {
                    addToList(currentList, plusMinusSymb); //within open (
                    if (!isOuter && isQuoteVal) {
                        (*currentIndex)++;
                        return plusMinusSymb;
                    }
                }
                else if (output != NULL) {
                    if (quoteList != NULL) {
                        quoteListTail = output;
                        if (quoteListTail->typeTag == TYPE_List) {
                            addToList(quoteListTail, plusMinusSymb);
                        }
                        output = copyValue(quoteList);
                        freeValue(&quoteList);
                        //quoteList = NULL;
                        quoteListTail = NULL;
                    }
                    else {
                        // error, two values but no list, do later, invalid token
                        badToken = badTokenCreator(startIndexCopy, userInput, currentIndex);
                        return invalidTokenError(badToken);
                    }
                }
                else {
                    output = plusMinusSymb;
                }
            }
            else {
                int nextAscii = (int)userInput[*currentIndex + 1];
                if (nextAscii >= 48 && nextAscii <= 57) {
                    (*currentIndex)++; //pass index of first digit, for consistency
                    isPositive = current == '+';
                    output = numberValidator(userInput, currentIndex, currentList, output, isPositive, &quoteList, isQuoteVal);
                    if (output->typeTag == TYPE_Error || (!isOuter && isQuoteVal)) {
                        return output;
                    }
                }
                else {
                    isAllSymb = 1;
                    output = symbolValidator(userInput, currentIndex, currentList, isAllSymb, isSpecial, output, &quoteList, isQuoteVal);
                    if (output->typeTag == TYPE_Error || (!isOuter && isQuoteVal)) {
                        return output;
                    }
                }
            }
        }
        else if (isSymbol(current)) {
            isAllSymb = 1;
            output = symbolValidator(userInput, currentIndex, currentList, isAllSymb, isSpecial, output, &quoteList, isQuoteVal);
            if (output->typeTag == TYPE_Error || (!isOuter && isQuoteVal)) {
                return output;
            }
        }
        else if (current == '&' || current == '_') {
            if (isspace(userInput[*currentIndex + 1]) || userInput[*currentIndex + 1] == '\0' ||
                userInput[*currentIndex + 1] == '(' || userInput[*currentIndex + 1] == ')') {
                //create symbol
                char specialString[2] = { current, '\0' };
                Value* specialSymbol = createSymbol(specialString);
                if (currentList != NULL) {
                    addToList(currentList, specialSymbol); //within open (
                    //(*currentIndex)++;
                    if (!isOuter && isQuoteVal) {
                        (*currentIndex)++;
                        return specialSymbol;
                    }
                }
                else if (output != NULL) {
                    if (quoteList != NULL) {
                        quoteListTail = output;
                        if (quoteListTail->typeTag == TYPE_List) {
                            addToList(quoteListTail, specialSymbol);
                        }
                        output = copyValue(quoteList);
                        freeValue(&quoteList);
                        quoteList = NULL;
                        quoteListTail = NULL;
                    }
                    else {
                        // error, two values but no list, do later, invalid token
                        badToken = badTokenCreator(startIndexCopy, userInput, currentIndex);
                        return invalidTokenError(badToken);
                    }
                }
                else {
                    output = specialSymbol;
                }
            }
            else {
                //create error, invalid token
                badToken = badTokenCreator(startIndexCopy, userInput, currentIndex);
                return invalidTokenError(badToken);
            }
        }
        else if (current == '#') {
            //if (*currentIndex < inputLength - 1) { //at least one element left

            //}
            if ((userInput[*currentIndex + 1] == 't') || (userInput[*currentIndex + 1] == 'f')) {
                Value* boolVar = NULL;
                if (userInput[*currentIndex + 1] == 't') {
                    if (!isspace(userInput[*currentIndex + 2]) && userInput[*currentIndex + 2] != '\0'
                        && userInput[*currentIndex + 2] != '(' && userInput[*currentIndex + 2] != ')') {
                        badToken = badTokenCreator(*currentIndex, userInput, currentIndex);
                        /*I can index at + 2, if + 1 is t, there is at least \0 left*/
                        return(invalidTokenError(badToken));
                    }
                    else {
                        _Bool inputTrue = 1;
                        boolVar = createBool(inputTrue);
                    }
                }
                else if (userInput[*currentIndex + 1] == 'f') {
                    if (!isspace(userInput[*currentIndex + 2]) && userInput[*currentIndex + 2] != '\0'
                        && userInput[*currentIndex + 2] != '(' && userInput[*currentIndex + 2] != ')') {
                        badToken = badTokenCreator(*currentIndex, userInput, currentIndex);
                        return(invalidTokenError(badToken));
                    }
                    else {
                        _Bool inputFalse = 0;
                        boolVar = createBool(inputFalse);
                    }
                }
                if (currentList != NULL) {
                    addToList(currentList, boolVar); //within open (
                    (*currentIndex) += 1; 
                    if (!isOuter && isQuoteVal) {
                        (*currentIndex) += 1; //have to increment by one again, won't increment after loop iteration since this returns
                        return boolVar;
                    }
                }
                else if (output != NULL) {
                    if (quoteList != NULL) {
                        quoteListTail = output;
                        if (quoteListTail->typeTag == TYPE_List) {
                            addToList(quoteListTail, boolVar);
                        }
                        output = copyValue(quoteList);
                        freeValue(&quoteList);
                        //quoteList = NULL;
                        quoteListTail = NULL;
                        (*currentIndex)++;
                    }
                    else {
                        // error, two values but no list, do later, invalid token
                        badToken = badTokenCreator(startIndexCopy, userInput, currentIndex);
                        return invalidTokenError(badToken);
                    }
                }
                else {
                    output = boolVar;
                    (*currentIndex)++;
                }
            }
            else {
                //error, invalid token
                badToken = badTokenCreator(*currentIndex, userInput, currentIndex);
                return invalidTokenError(badToken);
            }
        }
        else if (current == '(') {
            //handle output not being null on outer parse, should error with invalid token
            if (isOuter) {
                //add two cases, if quoteList isn't null, current output is quoteList's tail, so add the parenManager output to the current output list+
                if (output != NULL) {
                    if (quoteList != NULL) {
                        quoteListTail = output; //value of output is quoteList's tail, need to reassign it
                        output = parenManager(userInput, currentIndex);
                        (*currentIndex)--;
                        if (output->typeTag == TYPE_Error) {
                            return output; //just return the error
                        }
                        else {
                            addToList(quoteListTail, output);
                            output = copyValue(quoteList); //paren manager output isn't inaccessible, it's in the quoteList and will later be free'd
                            freeValue(&quoteList);
                            //quoteList = NULL;
                            quoteListTail = NULL;
                        }
                    }
                    else {
                        //badToken = badTokenCreator(startIndexCopy, userInput, currentIndex);
                        return invalidTokenError(userInput);
                    }
                }
                else {
                    output = parenManager(userInput, currentIndex);
                    //this covers first appearance only
                    //make sure to set errorFound = true if the Value* returned is of type Error
                    (*currentIndex)--;
                    if (output->typeTag == TYPE_Error) {
                        return output; //just return the error
                    }
                }
            }
            else {
                //represents recursion that needs to step into a new list, so return the current list at current index
                return currentList; //won't work, can't return here
            }
        }
        else if (current == '$') {
            //only can be output, ignore for now
        }
        else if (current == ';') {
            //comment, ignore for now
        }
        else if (current == '\"') {
            output = stringBuilder(userInput, currentIndex, currentList, output, &quoteList, isQuoteVal);
            if (output->typeTag == TYPE_Error) {
                return output;
            }
        }
        else if (current == '\'') {
            if (isOuter) {
                quoteList = createList();
                Value* quoteSymb = createSymbol("quote");
                addToList(quoteList, quoteSymb);
                (*currentIndex)++;
                output = rQuoteHelper(userInput, currentIndex, quoteList, *currentIndex, 1);
                (*currentIndex)--;
                if (output->typeTag == TYPE_Error) {
                    return output;
                }
            }
            else {
                return currentList; //represents quoting that the recursive caller needs to deal with
            }
        }
        else if (current == ')') {
            if (isOuter) { /*calls from the recursive parser to this function have balanced (), so ) is a closer in that case, not an error*/
                badToken = badTokenCreator(startIndexCopy, userInput, currentIndex);
                return invalidTokenError(badToken);
            }
            else {
                //return the list you have so far, it's done if called from the recursive parser
                return currentList;
            }
        }
        else if (!isspace(userInput[*currentIndex])) {
            //error, invalid token
            badToken = badTokenCreator(startIndexCopy, userInput, currentIndex);
            return invalidTokenError(badToken);
        }
        (*currentIndex)++;
        count++;
    }
    return output;
}

void* printValue(Value* input, _Bool addNewline, _Bool isError);

//RECURSIVE PARSER (LIST CREATOR)
Value* rParser(char* bufferIn, int* currentIndex, int64_t* offset, Value* myList, char* userInput, _Bool isQuoteVal) {
    //valid buffer is passed in with balanced parenthesis. Format is (DATADATADATA) AND MAY INCLUDE NESTED ()
    Value* returnListOrError;
    Value* rOutput;
    Value* quoteList = NULL;
    Value* lowerPointer = NULL;
    Value* quoteListCopy = NULL;
    _Bool isOuter = 0;
    while (bufferIn[*currentIndex - *offset] != ')') { // fix
        if (bufferIn[*currentIndex - *offset] == '(') { //fix
            (*currentIndex)++;
            Value* subList = createList();
            isQuoteVal = 0;
            rOutput = rParser(bufferIn, currentIndex, offset, subList, userInput, isQuoteVal);
            if (rOutput->typeTag == TYPE_Error) {
                return rOutput;
            }
            else {
                if (!quoteList) {
                    //if quote list is null, add to myList like above
                    addToList(myList, rOutput);
                }
                else {
                    //but if not null, intercept it, add return value to lowerPointer list and add quote list to myList
                    addToList(lowerPointer, rOutput);
                    quoteListCopy = copyValue(quoteList);
                    addToList(myList, quoteListCopy); //lowerPointer is the lowest list in quoteList, if no levels, they point to the same object
                    freeValue(&quoteList);
                    quoteList = NULL;
                    lowerPointer = NULL;
                }
            }
        }
        else if ((bufferIn[*currentIndex - *offset] == '\'')) { // move this else to a recursive helper that recursively handles quotes. It will call the parser returning 
            (*currentIndex)++;
            quoteList = createList();
            Value* quoteSymb = createSymbol("quote");
            addToList(quoteList, quoteSymb); //keep outer list here, call recursive quote handler, return a pointer to lowest level list needing to be added to
            
            lowerPointer = rQuoteHelper(userInput, currentIndex, quoteList, *currentIndex, isOuter);
            //make a list with quote in it, then recursively call this again.
            if (lowerPointer->typeTag == TYPE_Error) {
                return lowerPointer;
            }
            isQuoteVal = 1;
            rOutput = parser(userInput, currentIndex, lowerPointer, isOuter, isQuoteVal);
            //call parser with lowerPointer as the list
            //add quote list to myList after successful return, must parse non list and only one val
            if (rOutput->typeTag == TYPE_Error) {
                return rOutput;
            }
            else if (userInput[*currentIndex] != '(') { //if parser hits (, it returns the list passed in so recursive branch needs to handle it. Otherwise, valid value, add it
                quoteListCopy = copyValue(quoteList);
                addToList(myList, quoteListCopy);
                freeValue(&quoteList);
                quoteList = NULL;
                lowerPointer = NULL;
            }
        }
        else {
            isQuoteVal = 0;
            rOutput = parser(userInput, currentIndex, myList, isOuter, isQuoteVal);
            if (rOutput->typeTag == TYPE_Error) {
                return rOutput;
            } //for calls to the helper non recursive parser, no adding to the list is required, it happens in the parser
        }
    }
    (*currentIndex)++;
    //can't have error here, if parser parsed to ) and returns error, it will execute the error block in the loop
    return  myList;
}

//PARENTHESIS BUFFER CREATOR
Value* parenManager(char* userInput, int* currentIndex) {
    size_t bufferSize = 1;
    int64_t bufferIndex = 0;
    int64_t parenCountOpen = 1;
    int64_t parenCountClosed = 0;
    int64_t indexCopy = *currentIndex + 1;
    char currentChar = userInput[indexCopy];
    char* parenBuffer = malloc(bufferSize * sizeof(char));
    assert(parenBuffer);
    parenBuffer[bufferIndex] = '('; //open ( has to be included for any incomplete parse error printing later
    bufferIndex++;
    _Bool errorFound = 0;
    assert(parenBuffer);
    int64_t maxIterations = strlen(userInput) - *currentIndex;
    int64_t count = 1;
    Value* errorOut;
    char* temp = NULL;
    _Bool countParen = 1;
    while (parenCountOpen > parenCountClosed && count <= maxIterations) {
        if (currentChar == '\0') {
            //make incomplete parse error, do it outside the loop so '\0' gets added to buffer
            errorFound = 1;
        }
        else if (currentChar == '(' && countParen) {
            parenCountOpen++;
        }
        else if (currentChar == ')' && countParen) {
            parenCountClosed++;
        }
        else if (currentChar == '\"' && userInput[indexCopy - 1] != '\\') { 
            //checks for unescaped double quote, safe to check the decrement, a double quote cannot occur at index 0 in this function because it only is called when open ( is found
            countParen = !countParen;
        }

        bufferSize++;
        temp = realloc(parenBuffer, bufferSize * sizeof(char));
        assert(temp);
        parenBuffer = temp;

        parenBuffer[bufferIndex] = currentChar;
        bufferIndex++;
        indexCopy++;
        if (count < maxIterations) { //final iteration increments indexCopy one past the maximum index
            currentChar = userInput[indexCopy];
        }
        count++;
    }
    bufferSize++;
    temp = realloc(parenBuffer, bufferSize * sizeof(char));
    assert(temp);
    parenBuffer = temp;
    parenBuffer[bufferIndex] = '\0'; //add null terminator, bufferIndex was incremented
    if (errorFound) {
        errorOut = incompleteParseError(parenBuffer); //null terminator is included based on loop structure
        return errorOut;
    }
    else {
        Value* myList = createList();
        (*currentIndex)++; //increment past the first ( before calling the recursive parser
        int64_t offset = *currentIndex - 1;
        return rParser(parenBuffer, currentIndex, &offset, myList, userInput, 1); //pass list? def pass current index pointer
    }
    free(parenBuffer);
}

//EVALUATER SIGNATURE TO AVOID COMPILER ERROR
Value* evaluate(Value* input, Env* environment);

//RECURSIVE EVALUATOR
Value* rEvaluate(Value* currentList, Env* environment) {
    Value* output = NULL;
    int64_t count = 0;
    Value* evaluateList = createList();
    while (count < currentList->in.listVal->size) {
        Value* entry = currentList->in.listVal->data[count];
        if (entry->typeTag == TYPE_List) {
            output = rEvaluate(entry, environment);
            if (output->typeTag == TYPE_Error) {
                return output;
            }
            //add quit return so it gets passsed up
            if (output->typeTag == TYPE_Function && output->in.functionVal->funcTag == TYPE_builtin
                && strcmp(output->in.functionVal->funcName, "quit") == 0) {
                return output;
            }
            else if (output->typeTag == TYPE_List && currentList->in.listVal->size == 1) {
                return output; //empty lists can get returned, if parent list size is 1, then it's an empty list inside of an empty list, return it.
            }
            else {
                addToList(evaluateList, output); //non-list value, or empty list being added to a list with more than one element, add it
            }
        }
        else {
            if (entry->typeTag == TYPE_Symbol) {
                if (strcmp(entry->in.symbolVal, "def") == 0 && count == 0) {
                    if (currentList->in.listVal->size == 3) {
                        int64_t matchingIndex = -1;
                        if (currentList->in.listVal->data[1]->typeTag != TYPE_Symbol) {
                            return typeError(currentList->in.listVal->data[count]->in.symbolVal, 1,
                                "symbol", copyValue(currentList->in.listVal->data[1]));
                        }
                        else {
                            matchingIndex = lookupBuiltinIndex(environment, currentList->in.listVal->data[1]);
                                if (matchingIndex <= environment->protectedSize - 1 && matchingIndex >= 0) {
                                    return protectedSymbolError(currentList->in.listVal->data[1]->in.symbolVal); //copy not needed, it is copied in the error function
                                }
                        }
                        output = evaluate(currentList->in.listVal->data[2], environment);
                        if (output->typeTag == TYPE_Error) {
                            return output;
                        }
                        else {
                            if (matchingIndex >= 0) {
                                freeValue(&(environment->bindingArr[matchingIndex].matchedValue));
                                environment->bindingArr[matchingIndex].matchedValue = copyValue(output);
                            }
                            else {
                                Value* symbolString = copyValue(currentList->in.listVal->data[1]);
                                addToEnvironment(environment, symbolString->in.symbolVal, copyValue(output), 0);
                            }
                            return output;
                        }
                    }
                    else if (currentList->in.listVal->size == 4) {
                        //user defined, later
                    }
                    else {
                        return arityError(currentList->in.listVal->data[0]->in.symbolVal, "= 2 or =", 3,
                            currentList->in.listVal->size - 1);
                    }
                }
                else if (strcmp(entry->in.symbolVal, "if") == 0 && count == 0) {
                    if (currentList->in.listVal->size == 3 || currentList->in.listVal->size == 4) {
                        Value* output1 = evaluate(currentList->in.listVal->data[1], environment);
                        switch (output1->typeTag) {
                        case TYPE_Error:
                            return output1;
                            break;
                        case TYPE_Bool:
                            if (output1->in.boolVal == 0) {
                                if (currentList->in.listVal->size == 3) { //size is 2, no else form
                                    output = createBool(0);
                                }
                                else {
                                    output = evaluate(currentList->in.listVal->data[3], environment);
                                }
                            }
                            else {
                                output = evaluate(currentList->in.listVal->data[2], environment);
                            }
                            break;
                        default:
                            output = evaluate(currentList->in.listVal->data[2], environment);
                            break;
                        }
                        freeValue(&output1);
                        return output;
                    }
                    else {
                        return arityError(currentList->in.listVal->data[0]->in.symbolVal, "= 2 or =", 3,
                            currentList->in.listVal->size - 1);
                    }
                }
                else if (strcmp(entry->in.symbolVal, "quote") == 0) {
                    if (currentList->in.listVal->size != 2) {
                        return arityError(currentList->in.listVal->data[0]->in.symbolVal, "=", 1,
                            currentList->in.listVal->size - 1);
                    }
                    else {
                        return copyValue(currentList->in.listVal->data[1]);
                    }
                }
                else if (strcmp(entry->in.symbolVal, "head") == 0 || strcmp(entry->in.symbolVal, "tail") == 0) {
                    if (currentList->in.listVal->size != 2) {
                        return arityError(currentList->in.listVal->data[0]->in.symbolVal, "=", 1,
                            currentList->in.listVal->size - 1);
                    }
                    Value* output2 = evaluate(currentList->in.listVal->data[1], environment);
                    if (output2->typeTag == TYPE_Error) {
                        return output2;
                    }
                    else if (output2->typeTag != TYPE_List) {
                        return typeError(currentList->in.listVal->data[0]->in.symbolVal, 1, LIST,
                            output2);
                    }
                    else if (output2->in.listVal->size == 0) {
                        return valueError(currentList->in.listVal->data[0]->in.symbolVal,
                            output2);
                    }
                    else {
                        if (strcmp(entry->in.symbolVal, "head") == 0) {
                            return output2->in.listVal->data[0];
                        }
                        else {
                            Value* tailOutputList = createList();
                            for (int64_t i = 1; i < output2->in.listVal->size; i++) {
                                addToList(tailOutputList, copyValue(output2->in.listVal->data[i]));
                            }
                            freeValue(&output2);
                            return tailOutputList;
                        }
                    } // output2 gets returned in all blocks but tail, so does not need freeing there. Tail makes a new list
                    //because my lists are array backed, so then output2 needs to be free'd
                }
                else if (strcmp(entry->in.symbolVal, "cons") == 0) {
                    if (currentList->in.listVal->size != 3) {
                        return arityError(currentList->in.listVal->data[0]->in.symbolVal, "=", 2,
                            currentList->in.listVal->size - 1);
                    }
                    else {
                        Value* arg1 = evaluate(currentList->in.listVal->data[1], environment);
                        if (arg1->typeTag == TYPE_Error) {
                            return arg1;
                        }
                        Value* arg2 = evaluate(currentList->in.listVal->data[2], environment);
                        if (arg2->typeTag == TYPE_Error) {
                            return arg2;
                        }
                        if (arg2->typeTag != TYPE_List) {
                            freeValue(&arg1);
                            return typeError(currentList->in.listVal->data[0]->in.symbolVal, 1, LIST, arg2);
                        }
                        // prepend arg1 to arg2 list here
                        Value* consList = createList();
                        addToList(consList, copyValue(arg1));
                        for (int64_t j = 0; j < arg2->in.listVal->size; j++) {
                            addToList(consList, copyValue(arg2->in.listVal->data[j]));
                        }
                        freeValue(&arg1);
                        freeValue(&arg2);
                        return consList;
                    }
                }
                else if (strcmp(entry->in.symbolVal, "ord") == 0) {
                    if (currentList->in.listVal->size != 2) {
                        return (arityError(currentList->in.listVal->data[0]->in.symbolVal, "=", 1,
                            currentList->in.listVal->size - 1));
                    }
                    else {
                        if (currentList->in.listVal->data[1]->typeTag != TYPE_String) {
                            return (typeError(currentList->in.listVal->data[0]->in.symbolVal, 1, STRING,
                                copyValue(currentList->in.listVal->data[1])));
                        }
                        else {
                            Value* ordList = createList();
                            for (int k = 0; k < currentList->in.listVal->data[1]->in.stringVal->length; k++) {
                                int64_t number = (int)currentList->in.listVal->data[1]->in.stringVal->value[k];
                                Value* numberValue = createNumber(number);
                                if (number > 255 || number < 0) {
                                    freeValue(&ordList);
                                    return valueError(currentList->in.listVal->data[0]->in.symbolVal, numberValue);
                                }
                                else {
                                    addToList(ordList, numberValue);
                                }
                            }
                            return ordList;
                        }
                    }
                }
                else if (strcmp(entry->in.symbolVal, "chr") == 0) {
                    if (currentList->in.listVal->size != 2) {
                    return (arityError(currentList->in.listVal->data[0]->in.symbolVal, "=", 1,
                        currentList->in.listVal->size - 1));
                    }
                    else {
                        Value* output3 = evaluate(currentList->in.listVal->data[1], environment);
                        if (output3->typeTag == TYPE_Error) {
                            return output3;
                        }
                        else if (output3->typeTag != TYPE_List) {
                            return (typeError(currentList->in.listVal->data[0]->in.symbolVal, 1, LIST,
                                output3));
                        }
                        else {
                            char* runningString = malloc(sizeof(char));
                            assert(runningString);
                            int m = NULL;
                            for (m = 0; m < output3->in.listVal->size; m++) {
                                Value* badEntry = NULL;
                                if (output3->in.listVal->data[m]->typeTag != TYPE_Number) {
                                    badEntry = copyValue(output3->in.listVal->data[m]);
                                    free(runningString);
                                    runningString = NULL;
                                    freeValue(&output3);
                                    return (typeError(currentList->in.listVal->data[0]->in.symbolVal, m + 1, NUMBER,
                                        badEntry));
                                }
                                else {
                                    if (output3->in.listVal->data[m]->in.numberVal > 255 ||
                                            output3->in.listVal->data[m]->in.numberVal < 0) {
                                        badEntry = copyValue(output3->in.listVal->data[m]);
                                        free(runningString);
                                        runningString = NULL;
                                        freeValue(&output3);
                                        return valueError(currentList->in.listVal->data[0]->in.symbolVal,
                                            badEntry);
                                    }
                                    else {
                                        char entryChar = (char)output3->in.listVal->data[m]->in.numberVal;
                                        runningString[m] = entryChar;
                                        if (m < output3->in.listVal->size - 1) {
                                            Value* temp = realloc(runningString, (m + 2) * sizeof(char));
                                            assert(temp);
                                            runningString = temp;
                                        }
                                    }
                                }
                            }
                            freeValue(&output3);
                            return createString(runningString, m);
                        }
                    }
                }
                else if (strcmp(entry->in.symbolVal, "input") == 0) {
                    if (currentList->in.listVal->size != 1) {
                        return (arityError(currentList->in.listVal->data[0]->in.symbolVal, "=", 0,
                            currentList->in.listVal->size - 1));
                    }
                    else {
                        char userInput2[4096];
                        fgets(userInput2, sizeof(userInput2), stdin);
                        int newLineLoc = strcspn(userInput2, "\n");
                        userInput2[newLineLoc] = '\0';
                        return createString(userInput2, newLineLoc);
                    }
                }
                else if (strcmp(entry->in.symbolVal, "output") == 0) {
                    if (currentList->in.listVal->size < 2) {
                        return (arityError(currentList->in.listVal->data[0]->in.symbolVal, ">=", 1,
                            currentList->in.listVal->size - 1));
                    }
                    else {
                        for (int n = 1; n < currentList->in.listVal->size; n++) { //I print in the loop, so must check correct types of all entries first
                            if (currentList->in.listVal->data[n]->typeTag != TYPE_String) {
                                return typeError(currentList->in.listVal->data[0]->in.symbolVal, n + 1, STRING,
                                    copyValue(currentList->in.listVal->data[n]));
                            }
                        }
                        for (int p = 1; p < currentList->in.listVal->size; p++) {
                            Value* stringCopy = copyValue(currentList->in.listVal->data[p]); //I'm changing the string to null terminated, so copying it since would not be a correct LISP string if I modified the original
                            char* temp = realloc(stringCopy->in.stringVal->value, (stringCopy->in.stringVal->length + 1) * sizeof(char));
                            assert(temp);
                            stringCopy->in.stringVal->value = temp;
                            stringCopy->in.stringVal->value[stringCopy->in.stringVal->length] = '\0';
                            fputs(stringCopy->in.stringVal->value, stdout);
                            freeValue(&stringCopy);
                        }
                        return createBool(1);
                    }
                }
                else if (strcmp(entry->in.symbolVal, "type") == 0) {
                    if (currentList->in.listVal->size != 2) {
                        return (arityError(currentList->in.listVal->data[0]->in.symbolVal, "=", 1,
                            currentList->in.listVal->size - 1));
                    }
                    else {
                        Value* output4 = evaluate(currentList->in.listVal->data[1], environment);
                        if (output4->typeTag == TYPE_Error) {
                            return output4;
                        }
                        else {
                            Value* output5 = NULL;
                            switch (output4->typeTag) {
                            case TYPE_Number:
                                output5 = createSymbol(NUMBER);
                                break;
                            case TYPE_Bool:
                                output5 = createSymbol(BOOLEAN);
                                break;
                            case TYPE_Symbol:
                                output5 = createSymbol(SYMBOL);
                                break;
                            case TYPE_List:
                                output5 = createSymbol(LIST);
                                break;
                            case TYPE_Function:
                                output5 = createSymbol(FUNCTION);
                                break;
                            case TYPE_Error:
                                output5 = createSymbol(ERROR);
                                break;
                            case TYPE_String:
                                output5 = createSymbol(STRING);
                                break;
                            }
                            freeValue(&output4);
                            return output5;
                        }
                    }
                }
            }
            output = evaluate(entry, environment);
            if (output->typeTag == TYPE_Error) {
                freeValue(&evaluateList);
                return output;
            } //in a given recursive stack frame, quit can be added to a list. It gets evaluated, then returned
            else if (count == 0 && output->typeTag != TYPE_Function) {
                freeValue(&evaluateList);
                return headError();
            }
            else {
                addToList(evaluateList, output);
            }
        }
        count++;
    }
    if (evaluateList->in.listVal->size == 0) {
        return evaluateList; //covers empty nested lists
    }
    else {
        Value* functionValue = evaluateList->in.listVal->data[0];
        if (functionValue->typeTag != TYPE_Function) {
            freeValue(&evaluateList);
            return headError(); //covers case of empty lists outside of populated inner list(s)
        }
        else if (functionValue->in.functionVal->funcTag == TYPE_builtin) {
            output = functionValue->in.functionVal->in.builtIn(evaluateList, environment);
            freeValue(&evaluateList);
        }
        else {
            //user defined, do later
        }
        return output;
    }
}

//INNER AND OUTER EVALUATOR
Value* evaluate(Value* input, Env* environment) {
    Value* output = NULL;
    switch (input->typeTag) {
    case TYPE_List:
        if (input->in.listVal->size > 0) {
            output = rEvaluate(input, environment);
        }
        else {
            return copyValue(input);
        }
        break;
    case TYPE_Symbol: {
        //function or binding lookup
        Value* matchingLookup = lookupBuiltin(environment, input);
        if (!matchingLookup) {
            output = unboundSymbol(copyValue(input->in.symbolVal));
        }
        else {
            output = copyValue(matchingLookup); //need a deep copy, because it gets free'd in sublists, cannot use original environment variable
        }
        break;
    }
    default:
        output = copyValue(input);
    }
    return output;
}

//OUTERMOST PROCESSOR OF USER INPUT, DIRECTS TO READ AND EVALUATE HELPERS
Value* processor(const char* userInput, Env* environment) {
    Value* myList = NULL;
    int startingIndex = 0;
    int* p = &startingIndex;
    _Bool isOuter = 1;
    Value* output = parser(userInput, p, myList, isOuter, 0);
    if (output == NULL) {
        return NULL;
    }
    else if (output->typeTag == TYPE_Error) {
        return output;
    }
    else {
        //printValue(output, 1, 0);
        Value* temp = evaluate(output, environment);
        freeValue(&output); //this actually free's the argument passed in, i.e. the parser output, but input to evaluater
        output = temp; //new output is free'd later
    }
    return output;
}

void* printNumber(Value* input, _Bool addNewline) {
    if (addNewline) {
        printf("%lld\n", (int64_t)input->in.numberVal);
    }
    else {
        printf("%lld", (int64_t)input->in.numberVal);
    }
}

void* printBool(Value* input, _Bool addNewline) {
    if (input->in.boolVal) {
        if (addNewline) {
            puts("#t");
        }
        else {
            fputs("#t", stdout);
        }
    }
    else {
        if (addNewline) {
            puts("#f");
        }
        else {
            fputs("#f", stdout);
        }
    }
}

void* printSymbol(Value* input, _Bool addNewline) {
    if (addNewline) {
        puts(input->in.symbolVal);
    }
    else {
        fputs(input->in.symbolVal, stdout);
    }
}

//STUB FOR SIGNATURE OF OUTER PRINT VALUE FUNCTION

void* printList(Value* input, _Bool isError, _Bool isInner, int64_t previousIndex, int64_t previousSize) {
    if (input->in.listVal->size > 0 && input->in.listVal->data[0]->typeTag == TYPE_Symbol &&
            strcmp(input->in.listVal->data[0]->in.symbolVal, "quote") == 0) {
        putchar('\'');
        printValue(input->in.listVal->data[1], 0, isError); 
        //I don't see how a quote can wrap an error but I'm passing isError to maintain function design
    }
    else {
        if (isInner && previousIndex > 0) {
            putchar(' ');
        }
        putchar('(');
        for (int64_t i = 0; i < input->in.listVal->size; i++) {
            if (input->in.listVal->data[i]->typeTag == TYPE_List) {
                printList(input->in.listVal->data[i], isError, 1, i, input->in.listVal->size);
            }
            else {
                _Bool addNewline = 0;
                if (i > 0) {
                    putchar(' ');
                }
                printValue(input->in.listVal->data[i], addNewline, isError);
            }
        }
        putchar(')');
    }
}

void* printFunction(Value* input, _Bool addNewline) {
    if (addNewline) {
        puts(input->in.functionVal->funcName);
    }
    else {
        fputs(input->in.functionVal->funcName, stdout);
    }
}

void* printError(Value* input) {
    fputs("$error{", stdout);
    _Bool addNewline = 0;
    _Bool isError = 1;
    printValue(input->in.errorVal->wrappedValue, addNewline, isError);
    puts("}");
}

void* printString(Value* input, _Bool addNewline, _Bool isError) {
    if (isError) {
        for (int64_t i = 0; i < input->in.stringVal->length; i++) {
            putchar(input->in.stringVal->value[i]);
        }
        //fputs(input->in.stringVal, stdout);
    }
    else {
        putchar('\"');
        for (int64_t j = 0; j < input->in.stringVal->length; j++) {
            if (input->in.stringVal->value[j] == '\0') {
                fputs("\\x00", stdout);
            }
            else if (input->in.stringVal->value[j] == '\n') {
                fputs("\\n", stdout);
            }
            else if (input->in.stringVal->value[j] == '\"') {
                fputs("\\\"", stdout);
            }
            else if (input->in.stringVal->value[j] == '\\') {
                fputs("\\\\", stdout);
            }
            else {
                putchar(input->in.stringVal->value[j]);
            }
        }
        if (addNewline) {
            puts("\"");
        }
        else {
            putchar('\"');
        }
    }
}

//PRINT FUNCTION
void* printValue(Value* input, _Bool addNewline, _Bool isError) {
    switch (input->typeTag) {
    case TYPE_Number:
        printNumber(input, addNewline);
        break;
    case TYPE_Bool:
        printBool(input, addNewline);
        break;
    case TYPE_Symbol:
        printSymbol(input, addNewline);
        break;
    case TYPE_List:
        printList(input, isError, 0, 0, 0);
        if (addNewline) {
            putchar('\n');
        }
        break;
    case TYPE_Function:
        printFunction(input, addNewline);
        break;
    case TYPE_Error:
        printError(input);
        break;
    case TYPE_String:
        printString(input, addNewline, isError);
        break;
    }
}

int main(void) {
    enum {
        maxLength = 4096
    };
    char userInput[maxLength];
    _Bool active = 1;
    Env* environment = initEnvironment(); //create it here because it will be free'd here, pass it
    while (active) {
        printf("input> ");
        fgets(userInput, sizeof(userInput), stdin);
        if (userInput != NULL && strcmp(userInput, "\n") != 0) {
            int newLineLocation = strcspn(userInput, "\n"); //need to pass this location to parser for real length
            userInput[newLineLocation] = '\0';
            Value* output = processor(userInput, environment);
            if (output->typeTag == TYPE_Function && output->in.functionVal->funcTag == TYPE_builtin \
                && strcmp(output->in.functionVal->funcName, "quit") == 0) {
                active = 0;
            }
            else {
                _Bool addNewline = 1;
                _Bool isError = 0;
                printValue(output, addNewline, isError);
                if (output->typeTag != TYPE_Error) {
                    Value* underscoreSymb = createSymbol("_");
                    int64_t successIndex = lookupBuiltinIndex(environment, underscoreSymb);
                    if (successIndex >= 0) {
                        freeValue(&(environment->bindingArr[successIndex].matchedValue));
                        environment->bindingArr[successIndex].matchedValue = copyValue(output);
                    }
                    else {
                        if (environment->size == environment->protectedSize) {
                            addToEnvironment(environment, "_", copyValue(output), 1);
                        }
                        else {
                            char* stringToMove = environment->bindingArr[environment->protectedSize].funcString;
                            Value* ValToMove = copyValue(environment->bindingArr[environment->protectedSize].matchedValue);
                            addToEnvironment(environment, stringToMove, ValToMove, 0);
                            freeValue(&(environment->bindingArr[environment->protectedSize].matchedValue));
                            environment->bindingArr[environment->protectedSize].funcString = "_";
                            environment->bindingArr[environment->protectedSize].matchedValue = copyValue(output);
                            environment->protectedSize++;
                        }
                    }
                    freeValue(&underscoreSymb);
                }
                freeValue(&output);
            }
        }
    }
    if (environment) {
        freeEnvironment(&environment);
    }
    return 0;
}