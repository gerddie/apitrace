#ifndef OBJECTVISITOR_HPP
#define OBJECTVISITOR_HPP


namespace frametrim_reverse {

class ProgramObject;
class FramebufferObject;
class TexObject;
class BufObject;
class MatrixObject;
class ShaderObject;
class GlobalStateObject;
class VertexAttribArray;
class RenderbufferObject;
class BoundObject;

class ObjectVisitor {
public:
    virtual void visit(ProgramObject& obj) = 0;
    virtual void visit(FramebufferObject& obj) = 0;
    virtual void visit(TexObject& obj) = 0;
    virtual void visit(BufObject& obj) = 0;
    virtual void visit(MatrixObject& obj) = 0;
    virtual void visit(ShaderObject& obj) = 0;
    virtual void visit(GlobalStateObject& obj) = 0;
    virtual void visit(VertexAttribArray& obj) = 0;
    virtual void visit(RenderbufferObject& obj) = 0;
    virtual void visit(BoundObject& obj) = 0;
};

}

#endif // OBJECTVISITOR_HPP
