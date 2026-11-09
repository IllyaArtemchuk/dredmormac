// Repair false noreturn annotations on C++ allocator destructors in an analysis
// project. Does not change executable bytes. Usage: -postScript RepairAllocatorReturns.java
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.listing.FlowOverride;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.symbol.Reference;
import java.util.ArrayList;

public class RepairAllocatorReturns extends GhidraScript {
    public void run() throws Exception {
        ArrayList<Function> candidates=new ArrayList<>();
        for (Function f:currentProgram.getFunctionManager().getFunctions(true)) candidates.add(f);
        FunctionIterator external=currentProgram.getFunctionManager().getExternalFunctions();
        while (external.hasNext()) candidates.add(external.next());
        int count=0;
        for (Function f:candidates) {
            String name=f.getName(true);
            if (!name.contains("allocator") || !name.contains("~allocator")) continue;
            println("allocator destructor: "+f.getEntryPoint()+" "+name+" noreturn="+f.hasNoReturn());
            if (f.hasNoReturn()) {f.setNoReturn(false);count++;}
            Function target=f.getThunkedFunction(true);
            if (target!=null && target.hasNoReturn()) {target.setNoReturn(false);count++;}
            for (Reference ref:getReferencesTo(f.getEntryPoint())) {
                Instruction instruction=getInstructionAt(ref.getFromAddress());
                if (instruction!=null && instruction.getFlowType().isCall() && instruction.getFlowOverride()!=FlowOverride.NONE)
                    instruction.setFlowOverride(FlowOverride.NONE);
            }
        }
        println("Cleared "+count+" false noreturn flags");
        // Re-analysis immediately rediscovers the same false annotation in this
        // stripped import thunk. Export after this script without analyzeChanges.
    }
}
