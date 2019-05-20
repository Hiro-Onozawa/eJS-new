package ejsc;

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import ejsc.Main.Info;

public class SBCFileComposer {
    static class SBCInstruction {
        String insnName;
        String[] ops;

        SBCInstruction(String insnName, String... ops) {
            this.insnName = insnName;
            this.ops = ops;
        }
    }

    class SBCFunction extends CodeBuffer {
        int functionNumberOffset;

        /* function header */
        int callEntry;
        int sendEntry;
        int numberOfLocals;

        List<SBCInstruction> instructions;

        SBCFunction(BCBuilder.FunctionBCBuilder fb, int functionNumberOffset) {
            this.functionNumberOffset = functionNumberOffset;

            List<BCode> bcodes = fb.getInstructions();
            this.callEntry = fb.callEntry.dist(0);
            this.sendEntry = fb.sendEntry.dist(0);
            this.numberOfLocals = fb.numberOfLocals;

            instructions = new ArrayList<SBCInstruction>(bcodes.size());
            for (BCode bc: bcodes)
                bc.emit(this);
        }

        String getSuperInsnName(String insnName, SrcOperand... srcs) {
            String modifier = "";
            boolean hasConstantOperand = false;
            for (SrcOperand src: srcs) {
                if (src instanceof RegisterOperand)
                    modifier += "reg";
                else {
                    if (src instanceof FixnumOperand)
                        modifier += "fix";
                    else if (src instanceof FlonumOperand)
                        modifier += "flo";
                    else if (src instanceof StringOperand)
                        modifier += "str";
                    else if (src instanceof SpecialOperand)
                        modifier += "spec";
                    else
                        throw new Error("Unknown source operand");
                    hasConstantOperand = true;
                }
            }
            if (hasConstantOperand)
                return insnName + modifier;
            else
                return insnName;
        }

        String escapeString(String s) {
            return "\""+s+"\""; // TODO: do escape
        }

        String srcOperandField(SrcOperand src) {
            if (src instanceof RegisterOperand) {
                Register r = ((RegisterOperand) src).get();
                int n = r.getRegisterNumber();
                return Integer.toString(n);
            } else if (src instanceof FixnumOperand) {
                int n = ((FixnumOperand) src).get();
                return Integer.toString(n);
            } else if (src instanceof FlonumOperand) {
                double n = ((FlonumOperand) src).get();
                return Double.toString(n);
            } else if (src instanceof StringOperand) {
                String s = ((StringOperand) src).get();
                return escapeString(s);
            } else if (src instanceof SpecialOperand) {
                SpecialOperand.V v = ((SpecialOperand) src).get();
                switch (v) {
                case TRUE:
                    return "true";
                case FALSE:
                    return "false";
                case NULL:
                    return "null";
                case UNDEFINED:
                    return "undefined";
                default:
                    throw new Error("Unknown special");
                }
            } else
                throw new Error("Unknown source operand");
        }

        @Override
        void addFixnumSmallPrimitive(String insnName, Register dst, int n) {
            String a = Integer.toString(dst.getRegisterNumber());
            String b = Integer.toString(n);
            SBCInstruction insn = new SBCInstruction(insnName, a, b);
            instructions.add(insn);
        }
        @Override
        void addNumberBigPrimitive(String insnName, Register dst, double n) {
            String a = Integer.toString(dst.getRegisterNumber());
            String b = Double.toString(n);
            SBCInstruction insn = new SBCInstruction(insnName, a, b);
            instructions.add(insn);

        }
        @Override
        void addStringBigPrimitive(String insnName, Register dst, String s) {
            String a = Integer.toString(dst.getRegisterNumber());
            String b = escapeString(s);
            SBCInstruction insn = new SBCInstruction(insnName, a, b);
            instructions.add(insn);
        }
        @Override
        void addSpecialSmallPrimitive(String insnName, Register dst, SpecialValue v) {
            String a = Integer.toString(dst.getRegisterNumber());
            String b;
            switch (v) {
            case TRUE:
                b = "true"; break;
            case FALSE:
                b = "false"; break;
            case NULL:
                b = "null"; break;
            case UNDEFINED:
                b = "undefined"; break;
            default:
                throw new Error("Unknown special");
            }
            SBCInstruction insn = new SBCInstruction(insnName, a, b);
            instructions.add(insn);
        }
        @Override
        void addRegexp(String insnName, Register dst, int flag, String ptn) {
            String a = Integer.toString(dst.getRegisterNumber());
            String b = Integer.toString(flag);
            SBCInstruction insn = new SBCInstruction(insnName, a, b, ptn);
            instructions.add(insn);
        }
        @Override
        void addRXXThreeOp(String insnName, Register dst, SrcOperand src1, SrcOperand src2) {
            insnName = getSuperInsnName(insnName, src1, src2);
            String a = Integer.toString(dst.getRegisterNumber());
            String b = srcOperandField(src1);
            String c = srcOperandField(src2);
            SBCInstruction insn = new SBCInstruction(insnName, a, b, c);
            instructions.add(insn);
        }
        @Override
        void addXXXThreeOp(String insnName, SrcOperand src1, SrcOperand src2, SrcOperand src3) {
            insnName = getSuperInsnName(insnName, src1, src2, src3);
            String a = srcOperandField(src1);
            String b = srcOperandField(src2);
            String c = srcOperandField(src3);
            SBCInstruction insn = new SBCInstruction(insnName, a, b, c);
            instructions.add(insn);
        }
        @Override
        void addXIXThreeOp(String insnName, SrcOperand src1, int index, SrcOperand src2) {
            insnName = getSuperInsnName(insnName, src1, src2);
            String a = srcOperandField(src1);
            String b = Integer.toString(index);
            String c = srcOperandField(src2);
            SBCInstruction insn = new SBCInstruction(insnName, a, b, c);
            instructions.add(insn);
        }
        @Override
        void addRXTwoOp(String insnName, Register dst, SrcOperand src) {
            insnName = getSuperInsnName(insnName, src);
            String a = Integer.toString(dst.getRegisterNumber());
            String b = srcOperandField(src);
            SBCInstruction insn = new SBCInstruction(insnName, a, b);
            instructions.add(insn);
        }
        @Override
        void addXXTwoOp(String insnName, SrcOperand src1, SrcOperand src2) {
            insnName = getSuperInsnName(insnName, src1, src2);
            String a = srcOperandField(src1);
            String b = srcOperandField(src2);
            SBCInstruction insn = new SBCInstruction(insnName, a, b);
            instructions.add(insn);
        }
        @Override
        void addXRTwoOp(String insnName, SrcOperand src, Register dst) {
            insnName = getSuperInsnName(insnName, src);
            String a = srcOperandField(src);
            String b = Integer.toString(dst.getRegisterNumber());
            SBCInstruction insn = new SBCInstruction(insnName, a, b);
            instructions.add(insn);
        }
        @Override
        void addROneOp(String insnName, Register dst) {
            String a = Integer.toString(dst.getRegisterNumber());
            SBCInstruction insn = new SBCInstruction(insnName, a);
            instructions.add(insn);
        }
        @Override
        void addXOneOp(String insnName, SrcOperand src) {
            insnName = getSuperInsnName(insnName, src);
            String a = srcOperandField(src);
            SBCInstruction insn = new SBCInstruction(insnName, a);
            instructions.add(insn);
        }
        @Override
        void addIOneOp(String insnName, int n) {
            String a = Integer.toString(n);
            SBCInstruction insn = new SBCInstruction(insnName, a);
            instructions.add(insn);
        }
        @Override
        void addZeroOp(String insnName) {
            SBCInstruction insn = new SBCInstruction(insnName);
            instructions.add(insn);
        }
        @Override
        void addNewFrameOp(String insnName, int len, boolean mkargs) {
            String a = Integer.toString(len);
            String b = mkargs ? "1" : "0";
            SBCInstruction insn = new SBCInstruction(insnName, a, b);
            instructions.add(insn);
        }
        @Override
        void addGetVar(String insnName, Register dst, int link, int index) {
            String a = Integer.toString(dst.getRegisterNumber());
            String b = Integer.toString(link);
            String c = Integer.toString(index);
            SBCInstruction insn = new SBCInstruction(insnName, a, b, c);
            instructions.add(insn);
        }
        @Override
        void addSetVar(String insnName, int link, int index, SrcOperand src) {
            insnName = getSuperInsnName(insnName, src);
            String a = Integer.toString(link);
            String b = Integer.toString(index);
            String c = srcOperandField(src);
            SBCInstruction insn = new SBCInstruction(insnName, a, b, c);
            instructions.add(insn);
        }
        @Override
        void addMakeClosureOp(String insnName, Register dst, int index) {
            String a = Integer.toString(dst.getRegisterNumber());
            String b = Integer.toString(index + functionNumberOffset);
            SBCInstruction insn = new SBCInstruction(insnName, a, b);
            instructions.add(insn);
        }
        @Override
        void addXICall(String insnName, SrcOperand fun, int nargs) {
            insnName = getSuperInsnName(insnName, fun);
            String a = srcOperandField(fun);
            String b = Integer.toString(nargs);
            SBCInstruction insn = new SBCInstruction(insnName, a, b);
            instructions.add(insn);                    
        }
        @Override
        void addRXCall(String insnName, Register dst, SrcOperand fun) {
            insnName = getSuperInsnName(insnName, fun);
            String a = Integer.toString(dst.getRegisterNumber());
            String b = srcOperandField(fun);
            SBCInstruction insn = new SBCInstruction(insnName, a, b);
            instructions.add(insn);
        }
        @Override
        void addUncondJump(String insnName, int disp) {
            String b = Integer.toString(disp);
            SBCInstruction insn = new SBCInstruction(insnName, b);
            instructions.add(insn);
        }
        @Override
        void addCondJump(String insnName, SrcOperand test, int disp) {
            insnName = getSuperInsnName(insnName, test);
            String a = srcOperandField(test);
            String b = Integer.toString(disp);
            SBCInstruction insn = new SBCInstruction(insnName, a, b);
            instructions.add(insn);
        }
    }

    List<SBCFunction> obcFunctions;

    SBCFileComposer(BCBuilder compiledFunctions, int functionNumberOffset) {
        List<BCBuilder.FunctionBCBuilder> fbs = compiledFunctions.getFunctionBCBuilders();
        obcFunctions = new ArrayList<SBCFunction>(fbs.size());
        for (BCBuilder.FunctionBCBuilder fb: fbs) {
            SBCFunction out = new SBCFunction(fb, functionNumberOffset);
            obcFunctions.add(out);
        }
    }

    /**
     * Output instruction to the file.
     * @param fileName file name to be output to.
     */
    void output(String fileName) {
        try {
            FileOutputStream outs = new FileOutputStream(fileName);
            PrintWriter out = new PrintWriter(outs);

            /* File header */
            out.println("funcLength "+obcFunctions.size());

            /* Function */
            for (SBCFunction fun: obcFunctions) {
                /* Function header */
                out.println("callentry "+fun.callEntry);
                out.println("sendentry "+fun.sendEntry);
                out.println("numberOfLocals "+fun.numberOfLocals);
                out.println("numberOfInstruction "+fun.instructions.size());

                /* Instructions */
                for (SBCInstruction insn: fun.instructions) {
                    out.print(insn.insnName);
                    for (String op: insn.ops)
                        out.print(" "+op);
                    out.println();
                }
            }
            out.close();
        } catch (IOException e) {
            e.printStackTrace();
            System.exit(1);
        }
    }
}
