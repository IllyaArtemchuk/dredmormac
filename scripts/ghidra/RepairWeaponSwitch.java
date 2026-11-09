// Restore computed-jump references/overrides for both i386 weapon-type switches.
// Analysis annotations only; executable bytes are never changed.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.FlowOverride;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.symbol.RefType;
import ghidra.program.model.symbol.SourceType;
import ghidra.program.model.pcode.JumpTable;
import java.util.ArrayList;

public class RepairWeaponSwitch extends GhidraScript {
    public void run() throws Exception {
        Function function=getFunctionAt(toAddr(0x1397c0));
        if (function==null) throw new IllegalStateException("expected Weapon::EquipOnPlayer");
        restore(function,0x139903,0x139cf8,new long[]{0x139944,0x139905,0x13990c,0x139913,0x13991a,0x139921,0x139928,0x13992f,0x139936});
        restore(function,0x139add,0x139d1c,new long[]{0x139b1e,0x139adf,0x139ae6,0x139aed,0x139af4,0x139afb,0x139b02,0x139b09,0x139b10});
        println("Restored both weapon switch overrides; export without general re-analysis");
    }
    private void restore(Function function,long branch,long tableAddress,long[] expected) throws Exception {
        Address jump=toAddr(branch),table=toAddr(tableAddress);
        Instruction instruction=getInstructionAt(jump);
        if (function==null || instruction==null || !instruction.getMnemonicString().equalsIgnoreCase("JMP"))
            throw new IllegalStateException("expected supplied i386 Weapon::EquipOnPlayer switch");
        for (int i=0;i<expected.length;++i) {
            long target=0x1397d1L+currentProgram.getMemory().getInt(table.add(i*4));
            if (target!=expected[i]) throw new IllegalStateException("unexpected weapon switch table");
            Function owner=getFunctionContaining(toAddr(target));
            if (owner!=null && !owner.equals(function)) throw new IllegalStateException("switch target belongs to another function");
        }
        instruction.setFlowOverride(FlowOverride.BRANCH);
        AddressSet body=new AddressSet(function.getBody());body.addRange(toAddr(0x139905),toAddr(0x139cf7));
        function.setBody(body);
        ArrayList<Address> destinations=new ArrayList<>();
        for (long target:expected) {
            disassemble(toAddr(target));
            currentProgram.getReferenceManager().addMemoryReference(jump,toAddr(target),RefType.COMPUTED_JUMP,SourceType.USER_DEFINED,0);
            destinations.add(toAddr(target));
        }
        new JumpTable(jump,destinations,true,0).writeOverride(function);
    }
}
