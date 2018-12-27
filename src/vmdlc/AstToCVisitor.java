package vmdlc;

import nez.ast.Tree;
import nez.ast.TreeVisitorMap;
import nez.util.ConsoleUtils;
import nez.ast.Symbol;

import java.util.HashMap;
import java.util.Stack;
import java.util.ArrayList;
import java.util.Set;
import java.util.HashSet;
import java.util.List;
import java.lang.Exception;

import vmdlc.AstToCVisitor.DefaultVisitor;

import dispatch.DispatchProcessor;
import dispatch.RuleSetBuilder;
import dispatch.DispatchPlan;
import dispatch.RuleSet;
import type.VMDataType;

public class AstToCVisitor extends TreeVisitorMap<DefaultVisitor> {
    Stack<StringBuffer> outStack;

    public AstToCVisitor() {
        init(AstToCVisitor.class, new DefaultVisitor());
        outStack = new Stack<StringBuffer>();
    }

    public String start(Tree<?> node) {
        try {
            outStack.push(new StringBuffer());
            for (Tree<?> chunk : node) {
                visit(chunk, 0);
            }
            String program = outStack.pop().toString();
            return program;
        } catch (Exception e) {
            e.printStackTrace(System.out);
            return null;
        }
    }

    private final void visit(Tree<?> node, int indent) throws Exception {
        find(node.getTag().toString()).accept(node, indent);
    }

    private void print(Object o) {
        outStack.peek().append(o);
    }

    private void println(Object o) {
        outStack.peek().append(o + "\n");
    }

    private void printOperator(Tree<?> node, String s) throws Exception {
        Tree<?> leftNode = node.get(Symbol.unique("left"));
        Tree<?> rightNode = node.get(Symbol.unique("right"));
        print("(");
        visit(leftNode, 0);
        print(s);
        visit(rightNode, 0);
        print(")");
    }
    private void printIndent(int indent, String s) {
        for (int i = 0; i < indent; i++) {
            print("  ");
        }
        print(s);
    }
    private void printIndentln(int indent, String s) {
        printIndent(indent, s);
        println("");
    }

    public class DefaultVisitor {
        public void accept(Tree<?> node, int indent) throws Exception {
            for (Tree<?> seq : node) {
                visit(seq, indent);
            }
        }
    }

    public class PatternDefinition extends DefaultVisitor {
        public void accept(Tree<?> node, int indent) throws Exception {
        }
    }
    public class FunctionMeta extends DefaultVisitor {
        public void accept(Tree<?> node, int indent) throws Exception {
            Tree<?> bodyNode = node.get(Symbol.unique("definition"));
            visit(bodyNode, indent);
        }
    }
    public class FunctionDefinition extends DefaultVisitor {
        public void accept(Tree<?> node, int indent) throws Exception {
            Tree<?> bodyNode = node.get(Symbol.unique("body"));
            visit(bodyNode, indent);
        }
    }

    public class Block extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            for (Tree<?> seq : node) {
                visit(seq, indent + 1);
            }
        }
    }

    public class Match extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            Tree<?> params = node.get(Symbol.unique("params"));
            String[] formalParams = new String[params.size()];
            for (int i = 0; i < params.size(); i++) {
                formalParams[i] = params.get(i).toText();
            }

            Tree<?> cases = node.get(Symbol.unique("cases"));
            RuleSetBuilder rsb = new RuleSetBuilder(formalParams);
            List<RuleSetBuilder.CaseActionPair> caps = new ArrayList<RuleSetBuilder.CaseActionPair>();
            for (Tree<?> cas: cases) {
                if (cas.is(Symbol.unique("Case"))) {
                    Tree<?> pat = cas.get(Symbol.unique("pattern"));
                    RuleSetBuilder.Node rsbAst = toRsbAst(pat, rsb);
                    outStack.push(new StringBuffer());
                    Tree<?> statement = cas.get(Symbol.unique("body"));
                    visit(statement, 0);
                    String action = outStack.pop().toString();
                    caps.add(new RuleSetBuilder.CaseActionPair(rsbAst, action));
                } else if (cas.is(Symbol.unique("AnyCase"))) {
                    outStack.push(new StringBuffer());
                    Tree<?> statement = cas.get(Symbol.unique("body"));
                    visit(statement, 0);
                    String action = outStack.pop().toString();
                    caps.add(new RuleSetBuilder.CaseActionPair(new RuleSetBuilder.TrueNode(), action));                    
                }
            }
            RuleSet ruleSet = rsb.createRuleSet(caps);
            DispatchPlan dp = new DispatchPlan(formalParams.length, false);
            DispatchProcessor dispatchProcessor = new DispatchProcessor();
            dispatchProcessor.setLabelPrefix(node.getLineNum() + "_");
            String s = dispatchProcessor.translate(ruleSet, dp);
            println(s);
        }
        
        private RuleSetBuilder.Node toRsbAst(Tree<?> n, RuleSetBuilder rsb) {
            if (n.is(Symbol.unique("AndPattern"))) {
                RuleSetBuilder.Node left = toRsbAst(n.get(0), rsb);
                RuleSetBuilder.Node right = toRsbAst(n.get(1), rsb);
                return new RuleSetBuilder.AndNode(left, right);
            } else if (n.is(Symbol.unique("OrPattern"))) {
                RuleSetBuilder.Node left = toRsbAst(n.get(0), rsb);
                RuleSetBuilder.Node right = toRsbAst(n.get(1), rsb);
                return new RuleSetBuilder.OrNode(left, right);
            } else if (n.is(Symbol.unique("NotPattern"))) {
                RuleSetBuilder.Node child = toRsbAst(n.get(0), rsb);
                return new RuleSetBuilder.NotNode(child);
            } else if (n.is(Symbol.unique("TypePattern"))) {
                String opName = n.get(Symbol.unique("var")).toText();
                String tn = n.get(Symbol.unique("type")).toText().toLowerCase();
                VMDataType dt = VMDataType.get(tn);
                return rsb.new AtomicNode(opName, dt);
            }
            throw new Error("no such pattern");
        }
    }

    public class Return extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printIndent(indent, "return ");
            for (Tree<?> expr : node) {
                visit(expr, 0);
            }
            println(";");
        }
    }

    public class Assignment extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printIndent(indent, "");
            Tree<?> leftNode = node.get(Symbol.unique("left"));
            Tree<?> rightNode = node.get(Symbol.unique("right"));
            visit(leftNode, 0);
            print(" = ");
            visit(rightNode, 0);
            println(";");
        }
    }
    public class AssignmentPair extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printIndent(indent, "");
            Tree<?> leftNode = node.get(Symbol.unique("left"));
            Tree<?> rightNode = node.get(Symbol.unique("right"));
            Tree<?> fname = rightNode.get(Symbol.unique("recv"));
            print(fname.toText());
            print("(");
            
            for (Tree<?> child : rightNode) {
                if (child.is(Symbol.unique("ArgList"))) {
                    int i = 0;
                    for (i = 0; i < child.size(); i++) {
                        visit(child.get(i), 0);
                        print(", ");
                    }
                    int j = 0;
                    for (j = 0; j < leftNode.size() - 1; j++) {
                        print("&");
                        visit(leftNode.get(j), 0);
                        print(", ");
                    }
                    print("&");
                    visit(leftNode.get(j), 0);
                    
                    break;
                }
            }
            println(");");
        }
    }

    public class ExpressionStatement extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printIndent(indent, "");
            visit(node.get(0), indent);
            println(";");
        }
    }
    public class Declaration extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            Tree<?> typeNode = node.get(Symbol.unique("type"));
            Tree<?> varNode = node.get(Symbol.unique("var"));
            Tree<?> exprNode = node.get(Symbol.unique("expr"));
            visit(typeNode, 0);
            print(" ");
            visit(varNode, 0);
            print(" = ");
            visit(exprNode, 0);
            println(";");
        }
    }
    public class If extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            Tree<?> condNode = node.get(Symbol.unique("cond"));
            Tree<?> thenNode = node.get(Symbol.unique("then"));
            printIndent(indent, "if (");
            visit(condNode, indent + 1);
            println(") {");
            visit(thenNode, indent + 1);
            printIndentln(indent, "}");
            if (node.has(Symbol.unique("else"))) {
                Tree<?> elseNode = node.get(Symbol.unique("else"));
                printIndentln(indent, "else {");
                visit(elseNode, indent + 1);
                printIndentln(indent, "}");
            }
        }
    }
    public class Do extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            Tree<?> initNode = node.get(Symbol.unique("init"));
            Tree<?> stepNode = node.get(Symbol.unique("step"));
            Tree<?> blockNode = node.get(Symbol.unique("block"));
            printIndent(indent, "for (");
            visit(initNode, 0);
            Tree<?> varNode = initNode.get(Symbol.unique("var"));
            print(";;");
            visit(varNode, 0);
            print("=");
            visit(stepNode, 0);
            println(") {");
            visit(blockNode, indent + 1);
            printIndentln(indent, "}");
        }
    }
    public class DoInit extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            Tree<?> typeNode = node.get(Symbol.unique("type"));
            Tree<?> varNode = node.get(Symbol.unique("var"));
            Tree<?> exprNode = node.get(Symbol.unique("expr"));
            visit(typeNode, 0);
            print(" ");
            visit(varNode, 0);
            print(" = ");
            visit(exprNode, 0);
        }
    }

    public class Trinary extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            Tree<?> condNode = node.get(Symbol.unique("cond"));
            Tree<?> thenNode = node.get(Symbol.unique("then"));
            Tree<?> elseNode = node.get(Symbol.unique("else"));
            visit(condNode, 0);
            print(" ? ");
            visit(thenNode, 0);
            print(" : ");
            visit(elseNode, 0);
        }
    }
    public class Or extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printOperator(node, "||");
        }
    }
    public class And extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printOperator(node, "&&");
        }
    }
    public class BitwiseOr extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printOperator(node, "|");
        }
    }
    public class BitwiseXor extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printOperator(node, "^");
        }
    }
    public class BitwiseAnd extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printOperator(node, "&");
        }
    }
    public class Equals extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printOperator(node, "==");
        }
    }
    public class NotEquals extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printOperator(node, "!=");
        }
    }
    public class LessThanEquals extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printOperator(node, "<=");
        }
    }
    public class GreaterThanEquals extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printOperator(node, ">=");
        }
    }
    public class LessThan extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printOperator(node, "<");
        }
    }
    public class GreaterThan extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printOperator(node, ">");
        }
    }
    public class LeftShift extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printOperator(node, "<<");
        }
    }
    public class RightShift extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printOperator(node, ">>");
        }
    }
    public class Add extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printOperator(node, "+");
        }
    }
    public class Sub extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printOperator(node, "-");
        }
    }
    public class Mul extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printOperator(node, "*");
        }
    }
    public class Div extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printOperator(node, "/");
        }
    }
    public class Plus extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printOperator(node, "+");
        }
    }
    public class Minus extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printOperator(node, "-");
        }
    }
    public class Compl extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printOperator(node, "~");
        }
    }
    public class Not extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            printOperator(node, "!");
        }
    }
    public class Apply extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            visit(node.get(0), 0);
            print("(");
            for (int i = 1; i < node.size(); i++) {
                visit(node.get(i), 0);
            }
            print(")");
        }
    }
    public class ArgList extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            int i;
            for (i = 0; i < node.size() - 1; i++) {
                visit(node.get(i), 0);
                print(", ");
            }
            if (node.size() != 0) {
                visit(node.get(node.size() - 1), 0);
            }
        }
    }

    public class Index extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            visit(node.get(0), 0);
            print("[");
            visit(node.get(1), 0);
            print("]");

        }
    }
    public class Field extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            visit(node.get(0), 0);
            print(".");
            visit(node.get(1), 0);
        }
    }
    public class Float extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            print(node.toText());
        }
    }
    public class Integer extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            print(node.toText());
        }
    }
    public class _String extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            print("\"");
            print(node.toText());
            print("\"");
        }
    }
    public class _Character extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            print("\'");
            print(node.toText());
            print("\'");
        }
    }
    public class _True extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            print("1");
        }
    }
    public class _False extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            print("0");
        }
    }

    public class Name extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            print(node.toText());
        }
    }
    public class TypeName extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            print(node.toText());
        }
    }
    public class Ctype extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            HashMap<String, String> varmap = new HashMap<String, String>();
            varmap.put("cint", "int");
            varmap.put("cdouble", "double");
            print(varmap.get(node.toText()));
        }
    }
    public class CValue extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
            print("\"");
            print(node.toText());
            print("\"");
        }
    }
    
    /*
    public class Trinary extends DefaultVisitor {
        @Override
        public void accept(Tree<?> node, int indent) throws Exception {
        }
    }
    */
}
