package org.eclipse.tracecompass.veritracervisualizer.ui;

import java.util.Comparator;

import org.eclipse.tracecompass.tmf.ui.widgets.timegraph.model.ITimeGraphEntry;

/**
 * Comparators for two object from the columns content of the Tree in the GlobalView
 * @see org.eclipse.tracecompass.veritracervisualizer.ui.VeritraceGlobalViewColumnComparators
 * @author Damien Thenot
 *
 */
public interface IVeritraceTimeGraphEntryComparator {

	Comparator<ITimeGraphEntry> VARIABLE_NAME_COMPARATOR = new Comparator<ITimeGraphEntry>() {//Alphabetical comparison between o1 and o2
		@Override
		public int compare(ITimeGraphEntry o1, ITimeGraphEntry o2) {
			if(o1 == null || o2 == null || o1.getName() == null || o2.getName() == null) {
				throw new IllegalArgumentException();
			}
			return o1.getName().compareTo(o2.getName());
		}
	};

	Comparator<ITimeGraphEntry> TYPE_COMPARATOR = new Comparator<ITimeGraphEntry>() {//Integer comparison of the size of the variable represented by the entry o1 and o2
		@Override
		public int compare(ITimeGraphEntry o1, ITimeGraphEntry o2) {
			if (o1 == null || o2 == null) {
				throw new IllegalArgumentException();
			}
			int result = 0;
			if ((o1 instanceof VeritraceGlobalViewEntry) && (o2 instanceof VeritraceGlobalViewEntry)) {
				VeritraceGlobalViewEntry entry1 = (VeritraceGlobalViewEntry) o1;
				VeritraceGlobalViewEntry entry2 = (VeritraceGlobalViewEntry) o2;
				result = Integer.compare(entry1.getType(), entry2.getType());
			}
			return result;
		}
	};

	Comparator<ITimeGraphEntry> LINE_COMPARATOR = new Comparator<ITimeGraphEntry>() {//Integer comparison between line of variables  
		@Override
		public int compare(ITimeGraphEntry o1, ITimeGraphEntry o2) {
			if (o1 == null || o2 == null) {
				throw new IllegalArgumentException();
			}
			int result = 0;
			if ((o1 instanceof VeritraceGlobalViewEntry) && (o2 instanceof VeritraceGlobalViewEntry)) {
				VeritraceGlobalViewEntry entry1 = (VeritraceGlobalViewEntry) o1;
				VeritraceGlobalViewEntry entry2 = (VeritraceGlobalViewEntry) o2;
				result = Integer.compare(entry1.getLine(), entry2.getLine());
			}
			return result;
		}
	};
}
