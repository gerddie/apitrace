#ifndef OBJECTVISITOR_HPP
#define OBJECTVISITOR_HPP


namespace frametrim_reverse {

class ProgramObject;
class FramebufferObject;
class TexObject;
class BufObject;
class MatrixObject;

class ObjectVisitor
{
public:
    virtual void visit(ProgramObject& obj) = 0;
    virtual void visit(FramebufferObject& obj) = 0;
    virtual void visit(TexObject& obj) = 0;
    virtual void visit(BufObject& obj) = 0;
    virtual void visit(MatrixObject& obj) = 0;
};



}

#endif // OBJECTVISITOR_HPP
