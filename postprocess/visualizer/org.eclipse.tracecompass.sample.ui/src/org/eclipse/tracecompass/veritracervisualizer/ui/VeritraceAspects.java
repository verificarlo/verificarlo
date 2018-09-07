package org.eclipse.tracecompass.veritracervisualizer.ui;

import org.eclipse.tracecompass.statesystem.core.ITmfStateSystem;
import org.eclipse.tracecompass.statesystem.core.exceptions.AttributeNotFoundException;
import org.eclipse.tracecompass.statesystem.core.exceptions.StateSystemDisposedException;
import org.eclipse.tracecompass.tmf.core.event.ITmfEvent;
import org.eclipse.tracecompass.tmf.core.event.ITmfEventField;
import org.eclipse.tracecompass.tmf.core.event.aspect.ITmfEventAspect;
import org.eclipse.tracecompass.tmf.core.event.aspect.TmfEventFieldAspect;
import org.eclipse.tracecompass.tmf.core.statesystem.TmfStateSystemAnalysisModule;

/**
 * Set of aspects to show relevant informations in the columns of the EventView. 
 * @author Damien Thenot
 *
 */
public final class VeritraceAspects {
	/**
	 * Extract the variable name from the context of the variable on the State System.
	 */
	private static final ITmfEventAspect<String> VARIABLE_NAME_ASPECT = new ITmfEventAspect<String>() {
		@Override
		public String getName() {
			return "Variable Name";
		}

		@Override
		public String getHelpText() {
			return "The name of the variable in the original source file";
		}

		@Override
		public String resolve(ITmfEvent event) {
			if(event.getContent().getFieldNames().contains("context")) { //If the event has a reference to context and it could be used to link it to the context saved on the State System
				long context_id = (long) event.getContent().getField("context").getValue(); 
				final ITmfStateSystem ssq = TmfStateSystemAnalysisModule.getStateSystem(event.getTrace(), VeritraceAnalysisModule.ID); //We get the state system linked to this trace

				try {
					int quarkContextRoot = ssq.getQuarkAbsolute(Context.EventRoot); // We go at the root of the contexts in the State System
					int quarkContext = ssq.getQuarkRelative(quarkContextRoot, Long.toString(context_id)); //We obtain the quark identifying the correct context
					int quarkName = ssq.getQuarkRelative(quarkContext, "name"); // And get the quark of the attribute containing the information we need
					String name = ssq.querySingleState(ssq.getCurrentEndTime(), quarkName).getStateValue().unboxStr(); //And extract it
					return name;
				} catch (AttributeNotFoundException | StateSystemDisposedException e) {
					e.printStackTrace();
				}
			}
			//If the event is not available to extract a name, if it's not a value event, then nothing get extracted and printed in the column
			return null;
		}
	};

	/**
	 * Same as the other one but extract the type and gives a proper name for the type.
	 */
	private static final ITmfEventAspect<String> VARIABLE_TYPE_ASPECT = new ITmfEventAspect<String>() {
		@Override
		public String getName() {
			return "Variable Type";
		}

		@Override
		public String getHelpText() {
			return "Helptext VARIABLE_TYPE_ASPECT";
		}

		@Override
		public String resolve(ITmfEvent event) {
			if(event.getContent().getFieldNames().contains("context")) {
				long context_id = (long) event.getContent().getField("context").getValue();
				final ITmfStateSystem ssq = TmfStateSystemAnalysisModule.getStateSystem(event.getTrace(), VeritraceAnalysisModule.ID);

				try {
					int quarkContextRoot = ssq.getQuarkAbsolute(Context.EventRoot);
					int quarkContext = ssq.getQuarkRelative(quarkContextRoot, Long.toString(context_id));
					int quarkName = ssq.getQuarkRelative(quarkContext, "type");
					String type = ssq.querySingleState(ssq.getCurrentEndTime(), quarkName).getStateValue().unboxStr();
					return Context.getTypeName(Integer.parseInt(type)); //Get the name of the type
				} catch (AttributeNotFoundException | StateSystemDisposedException e) {
					e.printStackTrace();
				}
			}
			return null;
		}
	};
	
	/**
	 * Link the line from the context to the correct variable to show it for every event.
	 */
	private static final ITmfEventAspect<Integer> VARIABLE_LINE_ASPECT = new ITmfEventAspect<Integer>() {
		@Override
		public String getName() {
			return "Line";
		}

		@Override
		public String getHelpText() {
			return "The line of the instruction in the code";
		}
		
		@Override
		public Integer resolve(ITmfEvent event) {
			if(event.getContent().getFieldNames().contains("context")) {
				long context_id = (long) event.getContent().getField("context").getValue();
				final ITmfStateSystem ssq = TmfStateSystemAnalysisModule.getStateSystem(event.getTrace(), VeritraceAnalysisModule.ID);

				try {
					int quarkContextRoot = ssq.getQuarkAbsolute(Context.EventRoot);
					int quarkContext = ssq.getQuarkRelative(quarkContextRoot, Long.toString(context_id));
					int quarkName = ssq.getQuarkRelative(quarkContext, "line");
				
					Integer line = ssq.querySingleState(ssq.getCurrentEndTime(), quarkName).getStateValue().unboxInt(); // Extract the line from the correct place
					
					if(line == 0) { //If the line is 0, then the information could not be obtained during the processing of the trace and should not be shown
						return null; 
					}
					
					return line;
				} catch (AttributeNotFoundException | StateSystemDisposedException e) {
					e.printStackTrace();
				}
			}
			return null;
		}
	};
	
	
	
	/**
	 * Return the timestamp of the event to be shown.
	 */
	private static final ITmfEventAspect<Long> TIMESTAMP_ASPECT = new ITmfEventAspect<Long>() {
        @Override
        public String getName() {
            return "Timestamp";
        }

        @Override
        public String getHelpText() {
            return ITmfEventAspect.EMPTY_STRING;
        }

        @Override
        public Long resolve(ITmfEvent event) {
            return event.getTimestamp().toNanos();
        }
    };
    
    /**
     * Extract every content, this one is used to set hidden to true so the column does not appear.
     * @see org.eclipse.tracecompass.tmf.core.event.aspect.TmfBaseAspects.getContentsAspect
     */
    private static final TmfEventFieldAspect CONTENTS_ASPECT = new TmfEventFieldAspect("Content", null, new TmfEventFieldAspect.IRootField() {
        @Override
        public ITmfEventField getRootField(ITmfEvent event) {
            return event.getContent();
        }
    }) {
        @Override
        public String getHelpText() {
            return "";
        }
        
         @Override
         public boolean isHiddenByDefault() {
        	 return true;
         }
    };
    
    /**
     * Extract the name (or localpath + name) of the source file for this variable from the context.
     */
    private static final ITmfEventAspect<String> VARIABLE_FILE_ASPECT = new ITmfEventAspect<String>() {
		@Override
		public String getName() {
			return "File";
		}

		@Override
		public String getHelpText() {
			return "The name of the original source file";
			
		}

		@Override
		public String resolve(ITmfEvent event) {
			if(event.getContent().getFieldNames().contains("context")) {
				long context_id = (long) event.getContent().getField("context").getValue();
				final ITmfStateSystem ssq = TmfStateSystemAnalysisModule.getStateSystem(event.getTrace(), VeritraceAnalysisModule.ID);

				try {
					int quarkContextRoot = ssq.getQuarkAbsolute(Context.EventRoot);
					int quarkContext = ssq.getQuarkRelative(quarkContextRoot, Long.toString(context_id));
					int quarkName = ssq.getQuarkRelative(quarkContext, "file");
					String name = ssq.querySingleState(ssq.getCurrentEndTime(), quarkName).getStateValue().unboxStr();
					return name;
				} catch (AttributeNotFoundException | StateSystemDisposedException e) {
					e.printStackTrace();
				}
			}
			return null;
		}
	};
	
    public static TmfEventFieldAspect getContentsAspect() {
        return CONTENTS_ASPECT;
    }
    
    public static ITmfEventAspect<Long> getTimestampAspect(){
    	return TIMESTAMP_ASPECT;
    }
    
	public static ITmfEventAspect<String> getVariableNameAspect() {
		return VARIABLE_NAME_ASPECT;
	}

	public static ITmfEventAspect<String> getVariableTypeAspect() {
		return VARIABLE_TYPE_ASPECT;
	}
	
	public static ITmfEventAspect<Integer> getVariableLineAspect() {
		return VARIABLE_LINE_ASPECT;
	}
	
	public static ITmfEventAspect<String> getVariableFileAspect() {
		return VARIABLE_FILE_ASPECT;
	}
	
	
	/**
	 * Allow to extract informations which direcly exist in the event content and does not need to be linked to other existing information from the State System.
	 * @param nameContent name of the field in the CTF Trace event
	 * @param nameColumn name which will be shown to define the column
	 * @param helpText text which will be shown when the user hover the column
	 * @return the event aspect for the field
	 */
	public static ITmfEventAspect<String> getContentAspect(String nameContent, String nameColumn, String helpText){
		return new ITmfEventAspect<String>() {
			
			@Override
			public String getName() {
				return nameColumn;
			}

			@Override
			public String getHelpText() {
				return helpText;
			}

			@Override
			public String resolve(ITmfEvent event) {
				if(event.getContent().getFieldNames().contains(nameContent)) { //If the event possess a field of type nameContent it can be shown
					return event.getContent().getField(nameContent).getValue().toString();
				}
				return null; //else nothing is shown
			}
		};
	}
}
