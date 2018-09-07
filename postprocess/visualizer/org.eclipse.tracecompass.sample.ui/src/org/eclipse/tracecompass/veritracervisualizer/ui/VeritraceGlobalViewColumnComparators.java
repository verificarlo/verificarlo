package org.eclipse.tracecompass.veritracervisualizer.ui;

import java.util.Comparator;
import java.util.List;

import org.eclipse.swt.SWT;
import org.eclipse.tracecompass.tmf.ui.views.timegraph.ITimeGraphEntryComparator;
import org.eclipse.tracecompass.tmf.ui.widgets.timegraph.model.ITimeGraphEntry;

import com.google.common.collect.ImmutableList;

/**
 * Define the comparators for the columns of {@link VeritraceGlobalView}
 * @author Damien Thenot
 */
public class VeritraceGlobalViewColumnComparators {
	private VeritraceGlobalViewColumnComparators() {}

	/**
	 * Define the comparator for the variable column.
	 */
	public static final ITimeGraphEntryComparator VARIABLE_NAME_COLUMN_COMPARATOR = new ITimeGraphEntryComparator() {
		private final List<Comparator<ITimeGraphEntry>> SECONDARY_COMPARATORS = init();
		private int fDirection = SWT.DOWN; //direction of the sort

		@Override
		public int compare( ITimeGraphEntry o1,  ITimeGraphEntry o2) {
			//Sort by name
			int result = IVeritraceTimeGraphEntryComparator.VARIABLE_NAME_COMPARATOR.compare(o1, o2);
			return compareList(result, fDirection, SECONDARY_COMPARATORS, o1, o2);
		}

		@Override
		public void setDirection(int direction) {
			fDirection = direction;
		}

		private List<Comparator<ITimeGraphEntry>> init() {
			ImmutableList.Builder<Comparator<ITimeGraphEntry>> builder = ImmutableList.builder();
			builder.add(IVeritraceTimeGraphEntryComparator.VARIABLE_NAME_COMPARATOR)
			.add(IVeritraceTimeGraphEntryComparator.TYPE_COMPARATOR)
			.add(IVeritraceTimeGraphEntryComparator.LINE_COMPARATOR);
			return builder.build();
		}
	};
	
	/**
	 * Comparator for the columns containing the lines
	 */
	public static final ITimeGraphEntryComparator LINE_COLUMN_COMPARATOR = new ITimeGraphEntryComparator() {
		private final List<Comparator<ITimeGraphEntry>> SECONDARY_COMPARATORS = init();
		private int fDirection = SWT.DOWN;
		@Override
		public int compare(ITimeGraphEntry o1, ITimeGraphEntry o2) {
			//Sort by line
			int result = IVeritraceTimeGraphEntryComparator.LINE_COMPARATOR.compare(o1, o2);
			return compareList(result, fDirection, SECONDARY_COMPARATORS, o1, o2);
		}

		@Override
		public void setDirection(int direction) {
			fDirection = direction;
			
		}
		
		private List<Comparator<ITimeGraphEntry>> init() {
			ImmutableList.Builder<Comparator<ITimeGraphEntry>> builder = ImmutableList.builder();
			builder.add(IVeritraceTimeGraphEntryComparator.VARIABLE_NAME_COMPARATOR)
			.add(IVeritraceTimeGraphEntryComparator.TYPE_COMPARATOR)
			.add(IVeritraceTimeGraphEntryComparator.LINE_COMPARATOR);
			return builder.build();
		}
		
	};
	
	/**
	 * Comparison for the columns containing the type of an entry
	 */
	public static final ITimeGraphEntryComparator TYPE_COLUMN_COMPARATOR = new ITimeGraphEntryComparator() {
		private final List<Comparator<ITimeGraphEntry>> SECONDARY_COMPARATORS = init();
		private int fDirection = SWT.DOWN;

		@Override
		public int compare( ITimeGraphEntry o1,  ITimeGraphEntry o2) {
			//Sort by type
			int result = IVeritraceTimeGraphEntryComparator.TYPE_COMPARATOR.compare(o1, o2);
			return compareList(result, fDirection, SECONDARY_COMPARATORS, o1, o2);
		}

		@Override
		public void setDirection(int direction) {
			fDirection = direction;
		}

		private List<Comparator<ITimeGraphEntry>> init() {
			ImmutableList.Builder<Comparator<ITimeGraphEntry>> builder = ImmutableList.builder();
			builder.add(IVeritraceTimeGraphEntryComparator.VARIABLE_NAME_COMPARATOR)
			.add(IVeritraceTimeGraphEntryComparator.TYPE_COMPARATOR)
			.add(IVeritraceTimeGraphEntryComparator.LINE_COMPARATOR);
			return builder.build();
		}
	};

	private static int compareList(int prevResult, int direction, List<Comparator<ITimeGraphEntry>> comps, ITimeGraphEntry o1, ITimeGraphEntry o2) {
		int result = prevResult;
		for (Comparator<ITimeGraphEntry> comparator : comps) {
			if (result == 0) {
				result = comparator.compare(o1, o2);
				if (direction == SWT.UP) {
					result = -result;
				}
			}
		}
		return result;
	}
}
