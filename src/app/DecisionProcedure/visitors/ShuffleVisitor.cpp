//
// Created by Raph on 18/04/2016.
//

#include "ShuffleVisitor.h"
#include "../../Frontend/ast.h"
#include "../../Frontend/ast_visitor.h"

AST* ShuffleVisitor::visit(ASTForm_Ex1 *form) {
    auto result = form->f->accept(*this);
    form->f = reinterpret_cast<ASTForm*>(result);
    form->dag_height = form->f->dag_height + 1;
    return form;
}

AST* ShuffleVisitor::visit(ASTForm_Ex2 *form) {
    auto result = form->f->accept(*this);
    form->f = reinterpret_cast<ASTForm*>(result);
    form->dag_height = form->f->dag_height + 1;
    return form;
}

AST* ShuffleVisitor::visit(ASTForm_Not *form) {
    auto result = form->f->accept(*this);
    form->f = reinterpret_cast<ASTForm*>(result);
    form->dag_height = form->f->dag_height + 1;
    return form;
}

AST* ShuffleVisitor::visit(ASTForm_And *form) {
    return this->_visitBinary<ASTForm_And>(form);
}

AST* ShuffleVisitor::visit(ASTForm_Biimpl *form) {
    return this->_visitBinary<ASTForm_Biimpl>(form);
}

AST* ShuffleVisitor::visit(ASTForm_Impl *form) {
    return this->_visitBinary<ASTForm_Impl>(form);
}

AST* ShuffleVisitor::visit(ASTForm_Or *form) {
    return this->_visitBinary<ASTForm_Or>(form);
}

template<class BinopClass>
AST* ShuffleVisitor::_visitBinary(BinopClass*  form) {
    // No need to shuffle
    if(form->f1->kind != form->kind && form->f2->kind != form->kind) {
        return form;
    }
    // 1. Collect all lists for the chain of ff kind;
    // 2. Evaluate each of the lists
    LeafBuffer leaves;
    this->_CollectLeaves(form, form->kind, leaves);
    AST* left, *right;
    AST* newForm;

    // 3. Distribute evenly
    while(leaves.size() != 1) {
        assert(leaves.size() > 2);
        left = leaves.front();
        leaves.pop_front();
        right = leaves.front();
        leaves.pop_front();

        newForm = static_cast<AST*>(new BinopClass(static_cast<ASTForm*>(left), static_cast<ASTForm*>(right), Pos()));
        newForm->dag_height = std::max(left->dag_height, right->dag_height) + 1;
        this->_AddToBuffer(newForm, leaves);
    }

    // Return the last
    return leaves.front();
}

void ShuffleVisitor::_CollectLeaves(AST* form, ASTKind kind, LeafBuffer&leaves) {
    // Fixme: there should be deletion
    if(form->kind == kind) {
        ASTForm_ff* ff_form = reinterpret_cast<ASTForm_ff*>(form);
        this->_CollectLeaves(ff_form->f1, kind, leaves);
        this->_CollectLeaves(ff_form->f2, kind, leaves);
    } else {
        this->_AddFormToBuffer(form, leaves);
    }
}

void ShuffleVisitor::_AddFormToBuffer(AST* form, LeafBuffer& leaves) {
    auto result = static_cast<ASTForm*>(form)->accept(*this);
    this->_AddToBuffer(result, leaves);
}

void ShuffleVisitor::_AddToBuffer(AST* result, LeafBuffer& leaves) {
    for(auto it = leaves.begin(); it != leaves.end(); ++it) {
        if((*it)->dag_height > result->dag_height) {
            leaves.insert(it, result);
            return;
        }
    }
    leaves.insert(leaves.end(), result);
}