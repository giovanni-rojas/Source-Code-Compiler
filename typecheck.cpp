#include "typecheck.hpp"

// Defines the function used to throw type errors. The possible
// type errors are defined as an enumeration in the header file.
void typeError(TypeErrorCode code) {
  switch (code) {
    case undefined_variable:
      std::cerr << "Undefined variable." << std::endl;
      break;
    case undefined_method:
      std::cerr << "Method does not exist." << std::endl;
      break;
    case undefined_class:
      std::cerr << "Class does not exist." << std::endl;
      break;
    case undefined_member:
      std::cerr << "Class member does not exist." << std::endl;
      break;
    case not_object:
      std::cerr << "Variable is not an object." << std::endl;
      break;
    case expression_type_mismatch:
      std::cerr << "Expression types do not match." << std::endl;
      break;
    case argument_number_mismatch:
      std::cerr << "Method called with incorrect number of arguments." << std::endl;
      break;
    case argument_type_mismatch:
      std::cerr << "Method called with argument of incorrect type." << std::endl;
      break;
    case while_predicate_type_mismatch:
      std::cerr << "Predicate of while loop is not boolean." << std::endl;
      break;
    case do_while_predicate_type_mismatch:
      std::cerr << "Predicate of do while loop is not boolean." << std::endl;
      break;
    case if_predicate_type_mismatch:
      std::cerr << "Predicate of if statement is not boolean." << std::endl;
      break;
    case assignment_type_mismatch:
      std::cerr << "Left and right hand sides of assignment types mismatch." << std::endl;
      break;
    case return_type_mismatch:
      std::cerr << "Return statement type does not match declared return type." << std::endl;
      break;
    case constructor_returns_type:
      std::cerr << "Class constructor returns a value." << std::endl;
      break;
    case no_main_class:
      std::cerr << "The \"Main\" class was not found." << std::endl;
      break;
    case main_class_members_present:
      std::cerr << "The \"Main\" class has members." << std::endl;
      break;
    case no_main_method:
      std::cerr << "The \"Main\" class does not have a \"main\" method." << std::endl;
      break;
    case main_method_incorrect_signature:
      std::cerr << "The \"main\" method of the \"Main\" class has an incorrect signature." << std::endl;
      break;
  }
  exit(1);
}

// TypeCheck Visitor Functions: These are the functions you will
// complete to build the symbol table and type check the program.
// Not all functions must have code, many may be left empty.

void TypeCheck::visitProgramNode(ProgramNode* node) {
  classTable = new ClassTable();
  node->visit_children(this);
  
  if (classTable->count("Main") == 0)
  typeError(no_main_class);
  
}

void TypeCheck::visitClassNode(ClassNode* node) {

  ClassInfo cInfo; 
  currentClassName = node->identifier_1->name;
  if(node->identifier_2 != NULL) {
    // Check if that super class exists
    if(classTable->count(node->identifier_2->name) == 0) 
      typeError(undefined_class);
    cInfo.superClassName = node->identifier_2->name;
  }
  else 
    cInfo.superClassName = "";
  currentMethodTable = new MethodTable();
  currentVariableTable = new VariableTable();
  currentMemberOffset = 0; 
  currentLocalOffset = 0;

  std::string sc = cInfo.superClassName;
  while(sc!= "") {
    VariableTable *superVars = classTable->find(cInfo.superClassName)->second.members;
    sc = classTable->find(sc)->second.superClassName;
    for(std::map<std::string, VariableInfo>::iterator i = superVars->begin(); i != superVars->end(); i++) {
      VariableInfo vi = i->second;
      vi.offset = currentMemberOffset;
      (*currentVariableTable)[i->first] = vi;
      currentMemberOffset += 4;
    }
  }

  for(std::list<DeclarationNode*>::iterator i = node->declaration_list->begin(); i != node->declaration_list->end(); i++) 
    visitDeclarationNode(*i);
  cInfo.members = currentVariableTable;
  cInfo.membersSize = currentVariableTable->size()*4;

  if(currentClassName == "Main" && currentVariableTable->size() > 0)
    typeError(main_class_members_present);
  classTable->insert(std::pair<std::string, ClassInfo>(currentClassName, cInfo));

  for(std::list<MethodNode*>::iterator i = node->method_list->begin(); i != node->method_list->end(); i++){
    visitMethodNode(*i);
  }

  cInfo.methods = currentMethodTable;
  if(currentClassName == "Main" && currentMethodTable->count("main") == 0)
    typeError(no_main_method);
  std::map<std::string, ClassInfo>::iterator i;
  i = classTable->find(currentClassName);
  i->second.methods = currentMethodTable;
}

void TypeCheck::visitMethodNode(MethodNode* node) {
  
  MethodInfo mInfo;
  mInfo.parameters = new std::list<CompoundType>();
  CompoundType cT;
  currentVariableTable = new VariableTable();
  currentLocalOffset = -4;
  currentParameterOffset = 12;

  node->type->accept(this);
  cT.baseType = node->type->basetype;
  cT.objectClassName = node->type->objectClassName;
  mInfo.returnType = cT;

  for(std::list<ParameterNode*>::iterator i = node->parameter_list->begin(); i != node->parameter_list->end(); i++) {
    (*i)->type->accept(this);
    CompoundType ct;
    ct.baseType = (*i)->type->basetype;
    ct.objectClassName = (*i)->type->objectClassName;
    mInfo.parameters->push_back(ct);
    VariableInfo vInfo;
    vInfo.type = ct;
    vInfo.offset = currentParameterOffset;
    vInfo.size = 4;
    currentParameterOffset += 4;
    currentVariableTable->insert(std::pair<std::string, VariableInfo>((*i)->identifier->name, vInfo));
  }

  visitMethodBodyNode(node->methodbody);
  mInfo.variables = currentVariableTable;
  mInfo.localsSize = currentLocalOffset * (-1) - 4;

  if(node->type->basetype == bt_none && node->methodbody->returnstatement != NULL)
    typeError(return_type_mismatch);
  if(node->type->basetype != bt_none && node->methodbody->basetype != node->type->basetype) 
    typeError(return_type_mismatch);
  if(node->type->basetype == bt_object && node->methodbody->basetype == bt_object) {
    if(node->methodbody->objectClassName != node->type->objectClassName) {

      std::string sc = classTable->find(node->methodbody->objectClassName)->second.superClassName;
      while(sc != "") {
	if(sc != node->type->objectClassName) 
	  sc= classTable->find(sc)->second.superClassName;
	else 
	  break;
      }
      if(sc == "") 
	typeError(return_type_mismatch);
      else {}
    }
  }
  if(node->type->basetype != bt_none && node->methodbody->returnstatement == NULL)
    typeError(return_type_mismatch);
  if(currentClassName == node->identifier->name && node->type->basetype != bt_none)
    typeError(constructor_returns_type);
  if(currentClassName == "Main" && node->identifier->name == "main") {
    if(mInfo.parameters->size() > 0) 
      typeError(main_method_incorrect_signature);
  }

  currentMethodTable -> insert(std::pair<std::string, MethodInfo>(node->identifier->name, mInfo));
  std::map<std::string, ClassInfo>::iterator i;
  i = classTable->find(currentClassName);
  i->second.methods = currentMethodTable;
  
}

void TypeCheck::visitMethodBodyNode(MethodBodyNode* node) {
  
  node->visit_children(this);
  if (node->returnstatement != NULL){
    node->basetype = node->returnstatement->basetype;
    node->objectClassName = node->returnstatement->objectClassName;
  }
}

void TypeCheck::visitParameterNode(ParameterNode* node) {
  // WRITEME: Replace with code if necessary
  
  
}

void TypeCheck::visitDeclarationNode(DeclarationNode* node) {
  CompoundType cT;
  node->type->accept(this);
  cT.baseType = node->type->basetype;

  if(node->type->objectClassName != "") {
    if(classTable->count(node->type->objectClassName) == 0) 
      typeError(undefined_class);
    cT.objectClassName = node->type->objectClassName;
  }
  for(std::list<IdentifierNode*>::iterator i = node->identifier_list->begin(); i != node->identifier_list->end(); i++) {
    VariableInfo vInfo;
    vInfo.type = cT;
    vInfo.size = 4;
    (*i)->basetype = node->type->basetype;
    if(currentLocalOffset == 0) {
      vInfo.offset = currentMemberOffset;
      currentMemberOffset += 4; 
    }
    else {
      vInfo.offset = currentLocalOffset; 
      currentLocalOffset -= 4;
    }
    (*currentVariableTable)[(*i)->name] = vInfo;
  }
}

void TypeCheck::visitReturnStatementNode(ReturnStatementNode* node) {
  
  node->expression->accept(this);
  node->basetype = node->expression->basetype;
  node->objectClassName = node->expression->objectClassName;
}

void TypeCheck::visitAssignmentNode(AssignmentNode* node) {
  
  node->expression->accept(this);
  std::string className;
  CompoundType cT;
  cT.objectClassName = " ";

  if(currentVariableTable->count(node->identifier_1->name) > 0) {
    cT = currentVariableTable->find(node->identifier_1->name)->second.type;
  }
  else if (classTable->find(currentClassName)->second.members->count(node->identifier_1->name) > 0) {
    cT = classTable->find(currentClassName)->second.members->find(node->identifier_1->name)->second.type;
  }
  else {
    ClassInfo cI;
    cI = classTable->find(currentClassName)->second;
    std::string sc = cI.superClassName;
    while(sc!= "") {
      cI = classTable->find(sc)->second;
      if(cI.members->count(node->identifier_1->name) > 0) {
	cT = cI.members->find(node->identifier_1->name)->second.type;
	break;
      }
      sc = cI.superClassName;
    }
  }
  if(node->identifier_2 == NULL) {
    if(cT.objectClassName != " ") {
      node->basetype = cT.baseType;
      node->objectClassName = cT.objectClassName;
    }
    else
      typeError(undefined_variable);
  }
  else {
    if(cT.objectClassName == " ")
      typeError(undefined_variable);
    if(cT.baseType != bt_object) 
      typeError(not_object);
    if(classTable->find(cT.objectClassName)->second.members->count(node->identifier_2->name) > 0) {
      node->basetype = classTable->find(cT.objectClassName)->second.members->find(node->identifier_2->name)->second.type.baseType;
      node->objectClassName = classTable->find(cT.objectClassName)->second.members->find(node->identifier_2->name)->second.type.objectClassName;
    }
    else 
      typeError(undefined_member);
  }
  if(node->basetype != node->expression->basetype || node->objectClassName != node->expression->objectClassName) typeError(assignment_type_mismatch);
}

void TypeCheck::visitCallNode(CallNode* node) {
  node->visit_children(this);
  node->basetype = node->methodcall->basetype;
  node->objectClassName = node->methodcall->objectClassName;
}

void TypeCheck::visitIfElseNode(IfElseNode* node) {
  
  node->visit_children(this);
  if (node->expression->basetype != bt_boolean)
  typeError(if_predicate_type_mismatch);
  
}

void TypeCheck::visitWhileNode(WhileNode* node) {
  
  node->visit_children(this);
  if (node->expression->basetype != bt_boolean)
  typeError(while_predicate_type_mismatch);
}

void TypeCheck::visitDoWhileNode(DoWhileNode* node) {
 
  node->visit_children(this);
  if (node->expression->basetype != bt_boolean)
  typeError(do_while_predicate_type_mismatch);
}

void TypeCheck::visitPrintNode(PrintNode* node) {
  node->visit_children(this);
}

void TypeCheck::visitPlusNode(PlusNode* node) {
  
  node->visit_children(this);
  if (node->expression_1->basetype != bt_integer || node->expression_2->basetype != bt_integer)
  typeError(expression_type_mismatch);
  node->basetype = bt_integer;
}

void TypeCheck::visitMinusNode(MinusNode* node) {
  
  if (node->expression_1->basetype != bt_integer || node->expression_2->basetype != bt_integer)
    typeError(expression_type_mismatch);
  node->basetype = bt_integer; 
}

void TypeCheck::visitTimesNode(TimesNode* node) {
  
  if (node->expression_1->basetype != bt_integer || node->expression_2->basetype != bt_integer)
    typeError(expression_type_mismatch);
  node->basetype = bt_integer; 
}

void TypeCheck::visitDivideNode(DivideNode* node) {
  
  if (node->expression_1->basetype != bt_integer || node->expression_2->basetype != bt_integer)
    typeError(expression_type_mismatch);
  node->basetype = bt_integer; 
}

void TypeCheck::visitGreaterNode(GreaterNode* node) {
  
  node->visit_children(this);
  if (node->expression_1->basetype != bt_integer || node->expression_2->basetype != bt_integer)
    typeError(expression_type_mismatch);
  node->basetype = bt_boolean; 
}

void TypeCheck::visitGreaterEqualNode(GreaterEqualNode* node) {
  
   node->visit_children(this);
  if (node->expression_1->basetype != bt_integer || node->expression_2->basetype != bt_integer)
    typeError(expression_type_mismatch);
  node->basetype = bt_boolean; 
}

void TypeCheck::visitEqualNode(EqualNode* node) {
  
  node->visit_children(this);
  if (node->expression_1->basetype != node->expression_2->basetype)
    typeError(expression_type_mismatch);
  if (node->expression_1->basetype == bt_none || node->expression_1->basetype == bt_object)
    typeError(expression_type_mismatch);
  node->basetype = bt_boolean;
}

void TypeCheck::visitAndNode(AndNode* node) {
  
  node->visit_children(this);
  if (node->expression_1->basetype != bt_boolean || node->expression_2->basetype != bt_boolean)
    typeError(expression_type_mismatch);
  node->basetype = bt_boolean;
}

void TypeCheck::visitOrNode(OrNode* node) {
  
  node->visit_children(this);
  if (node->expression_1->basetype != bt_boolean || node->expression_2->basetype != bt_boolean)
    typeError(expression_type_mismatch);
  node->basetype = bt_boolean;
}

void TypeCheck::visitNotNode(NotNode* node) {
  
  node->expression->accept(this);
  if (node->expression->basetype != bt_boolean)
    typeError(expression_type_mismatch);
  node->basetype = bt_boolean;
}

void TypeCheck::visitNegationNode(NegationNode* node) {
  
  node->expression->accept(this);
  if (node->expression->basetype != bt_integer)
    typeError(expression_type_mismatch);
  node->basetype = bt_integer;
}

void TypeCheck::visitMethodCallNode(MethodCallNode* node) {
  
   node->visit_children(this);
  MethodInfo mInfo;
  mInfo.localsSize = -1;
  if (node->identifier_2 == NULL) {
    if (currentMethodTable -> count(node->identifier_1->name) > 0)
      mInfo = currentMethodTable->find(node->identifier_1->name)->second;    
    ClassInfo cInfo;
    cInfo = classTable->find(currentClassName)->second;
    std::string sc = cInfo.superClassName;
    while (sc != "") {
      cInfo = classTable->find(sc)->second;
      if (cInfo.methods->count(node->identifier_1->name) > 0) {
	mInfo = cInfo.methods->find(node->identifier_1->name)->second;
	break;
      }
      sc = cInfo.superClassName;
    }
  } 
  else {
    CompoundType cT;
    cT.objectClassName = " ";
        
    if (currentVariableTable->count(node->identifier_1->name) > 0) {
      cT = currentVariableTable->find(node->identifier_1->name)->second.type;
    }
    else if (classTable->find(currentClassName)->second.members->count(node->identifier_1->name) > 0) {
      cT = classTable->find(currentClassName)->second.members->find(node->identifier_1->name)->second.type;
    } 
    else {    
      ClassInfo cInfo;
      cInfo = classTable->find(currentClassName)->second;
      std::string sc = cInfo.superClassName;
      while (sc != "") {
	cInfo = classTable->find(sc)->second;          
	if (cInfo.members->count(node->identifier_1->name) > 0) {
	  cT = cInfo.members->find(node->identifier_1->name)->second.type;
	  break;
	}
	sc = cInfo.superClassName;
      }
    }
    if (cT.objectClassName == " " || cT.baseType != bt_object) {
      typeError(not_object);
      return;
    }
    if (classTable->find(cT.objectClassName)->second.methods->count(node->identifier_2->name) > 0) {
      mInfo = classTable->find(cT.objectClassName)->second.methods->find(node->identifier_2->name)->second;
    } 
    else {
      ClassInfo cInfo;
      cInfo = classTable->find(cT.objectClassName)->second;
      std::string sc = cInfo.superClassName;
      std::string obj = node->identifier_2->name;
      while (sc != "") {
	cInfo = classTable->find(sc)->second;           
	if (cInfo.methods->count(obj) > 0) {
	  mInfo = cInfo.methods->find(obj)->second;
	  break;
	}
	sc = cInfo.superClassName;
      }
    }
  }
  if (mInfo.localsSize == -1) {
    typeError(undefined_method);
    return;
  }
  if (mInfo.parameters->size() != node->expression_list->size()) {
    typeError(argument_number_mismatch);
    return;
  }
  std::list<CompoundType>::iterator mParams = mInfo.parameters->begin();
  for(std::list<ExpressionNode*>::iterator nodeParams = node->expression_list->begin(); nodeParams != node->expression_list->end(); nodeParams++){
    if ((*nodeParams)->basetype != (*mParams).baseType || (*nodeParams)->objectClassName != (*mParams).objectClassName) 
      typeError(argument_type_mismatch);
    mParams++;
  }
  node->basetype = mInfo.returnType.baseType;
  node->objectClassName = mInfo.returnType.objectClassName;
}


void TypeCheck::visitMemberAccessNode(MemberAccessNode* node) {
  
  CompoundType cT;
  cT.objectClassName = " ";
    
  if (currentVariableTable->count(node->identifier_1->name) > 0)
    cT = currentVariableTable->find(node->identifier_1->name)->second.type;
  if (classTable->find(currentClassName)->second.members->count(node->identifier_1->name) > 0) {
    cT = classTable->find(currentClassName)->second.members->find(node->identifier_1->name)->second.type;
  } 
  else {
    ClassInfo cInfo;
    cInfo = classTable->find(currentClassName)->second;
    std::string sc = cInfo.superClassName;
    while (sc != "") {
      cInfo = classTable->find(sc)->second;
      if (cInfo.members->count(node->identifier_1->name) > 0) {
	cT = cInfo.members->find(node->identifier_1->name)->second.type;
	break;
      }
      sc = cInfo.superClassName;
    }
  }
  if (cT.baseType != bt_object || cT.objectClassName == " ") {
    typeError(not_object);
    return;
  }
  if (classTable->find(cT.objectClassName)->second.members->count(node->identifier_2->name) > 0) {
    cT = classTable->find(cT.objectClassName)->second.members->find(node->identifier_2->name)->second.type;
  }
  else {
    ClassInfo cInfo;
    cInfo.membersSize = -1;
    cInfo = classTable->find(cT.objectClassName)->second;
    std::string sc = cInfo.superClassName;
    std::string obj = node->identifier_2->name;
    cT.objectClassName = " ";
    while (sc != "") {
      cInfo = classTable->find(sc)->second;
      if (cInfo.members->count(obj) > 0) {
	cT = cInfo.members->find(obj)->second.type;
	break;
      }
      sc = cInfo.superClassName;
    }
    if (cT.objectClassName == " ") {
      typeError(undefined_member);
      return;
    }
  }
  node->basetype = cT.baseType;
  node->objectClassName = cT.objectClassName;
}

void TypeCheck::visitVariableNode(VariableNode* node) {
  
  node->visit_children(this);
  CompoundType cT;
  cT.objectClassName = " ";
    
  if (currentVariableTable->count(node->identifier->name) > 0)
    cT = currentVariableTable->find(node->identifier->name)->second.type;
  if (classTable->find(currentClassName)->second.members->count(node->identifier->name)>0) {
    cT = classTable->find(currentClassName)->second.members->find(node->identifier->name)->second.type;
  } 
  else {
    ClassInfo cInfo;
    cInfo = classTable->find(currentClassName)->second;
    std::string sc = cInfo.superClassName;
    while (sc != "") {
      cInfo = classTable->find(sc)->second;
      if (cInfo.members->count(node->identifier->name) > 0) {
	cT = cInfo.members->find(node->identifier->name)->second.type;
	break;
      }
      sc = cInfo.superClassName;
    }
  }
  if (cT.objectClassName != " ") {
    node->basetype = cT.baseType;
    node->objectClassName = cT.objectClassName;
  } 
  else{
    typeError(undefined_variable);
  }
}

void TypeCheck::visitIntegerLiteralNode(IntegerLiteralNode* node) {
  node->basetype = bt_integer;
  node->objectClassName = "";
}

void TypeCheck::visitBooleanLiteralNode(BooleanLiteralNode* node) {
  
  node->basetype = bt_boolean;
  node->objectClassName = "";
}

void TypeCheck::visitNewNode(NewNode* node) {
  
   node->visit_children(this);
    
  if (classTable->count(node->identifier->name) == 0) {
    typeError(undefined_class);
    return;
  }
  
  if(node->expression_list != NULL) {  
    MethodInfo mInfo= classTable->find(node->identifier->name)->second.methods->find(node->identifier->name)->second;
    if (mInfo.parameters->size() != node->expression_list->size()) 
      typeError(argument_number_mismatch);
    std::list<CompoundType>::iterator mParams = mInfo.parameters->begin();
    for(std::list<ExpressionNode*>::iterator nodeParams = node->expression_list->begin(); nodeParams != node->expression_list->end(); nodeParams++) {
      if ((*nodeParams)->basetype != (*mParams).baseType || (*nodeParams)->objectClassName != (*mParams).objectClassName)
	typeError(argument_type_mismatch);
      mParams++;
    }
  }
  node->basetype = bt_object;
  node->objectClassName = node->identifier->name;
}

void TypeCheck::visitIntegerTypeNode(IntegerTypeNode* node) {
  
  node->basetype = bt_integer;
  node->objectClassName = "";
}


void TypeCheck::visitBooleanTypeNode(BooleanTypeNode* node) {
  node->basetype = bt_boolean;
  node->objectClassName = "";
}

void TypeCheck::visitObjectTypeNode(ObjectTypeNode* node) {
  
  node->basetype = bt_object;
  node->objectClassName = node->identifier->name;
}

void TypeCheck::visitNoneNode(NoneNode* node) {
  
  node->basetype = bt_none;
  node->objectClassName = "";
}

void TypeCheck::visitIdentifierNode(IdentifierNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitIntegerNode(IntegerNode* node) {
  // WRITEME: Replace with code if necessary
}


// The following functions are used to print the Symbol Table.
// They do not need to be modified at all.

std::string genIndent(int indent) {
  std::string string = std::string("");
  for (int i = 0; i < indent; i++)
    string += std::string(" ");
  return string;
}

std::string string(CompoundType type) {
  switch (type.baseType) {
    case bt_integer:
      return std::string("Integer");
    case bt_boolean:
      return std::string("Boolean");
    case bt_none:
      return std::string("None");
    case bt_object:
      return std::string("Object(") + type.objectClassName + std::string(")");
    default:
      return std::string("");
  }
}


void print(VariableTable variableTable, int indent) {
  std::cout << genIndent(indent) << "VariableTable {";
  if (variableTable.size() == 0) {
    std::cout << "}";
    return;
  }
  std::cout << std::endl;
  for (VariableTable::iterator it = variableTable.begin(); it != variableTable.end(); it++) {
    std::cout << genIndent(indent + 2) << it->first << " -> {" << string(it->second.type);
    std::cout << ", " << it->second.offset << ", " << it->second.size << "}";
    if (it != --variableTable.end())
      std::cout << ",";
    std::cout << std::endl;
  }
  std::cout << genIndent(indent) << "}";
}

void print(MethodTable methodTable, int indent) {
  std::cout << genIndent(indent) << "MethodTable {";
  if (methodTable.size() == 0) {
    std::cout << "}";
    return;
  }
  std::cout << std::endl;
  for (MethodTable::iterator it = methodTable.begin(); it != methodTable.end(); it++) {
    std::cout << genIndent(indent + 2) << it->first << " -> {" << std::endl;
    std::cout << genIndent(indent + 4) << string(it->second.returnType) << "," << std::endl;
    std::cout << genIndent(indent + 4) << it->second.localsSize << "," << std::endl;
    print(*it->second.variables, indent + 4);
    std::cout <<std::endl;
    std::cout << genIndent(indent + 2) << "}";
    if (it != --methodTable.end())
      std::cout << ",";
    std::cout << std::endl;
  }
  std::cout << genIndent(indent) << "}";
}

void print(ClassTable classTable, int indent) {
  std::cout << genIndent(indent) << "ClassTable {" << std::endl;
  for (ClassTable::iterator it = classTable.begin(); it != classTable.end(); it++) {
    std::cout << genIndent(indent + 2) << it->first << " -> {" << std::endl;
    if (it->second.superClassName != "")
      std::cout << genIndent(indent + 4) << it->second.superClassName << "," << std::endl;
    print(*it->second.members, indent + 4);
    std::cout << "," << std::endl;
    print(*it->second.methods, indent + 4);
    std::cout <<std::endl;
    std::cout << genIndent(indent + 2) << "}";
    if (it != --classTable.end())
      std::cout << ",";
    std::cout << std::endl;
  }
  std::cout << genIndent(indent) << "}" << std::endl;
}

void print(ClassTable classTable) {
  print(classTable, 0);
}
