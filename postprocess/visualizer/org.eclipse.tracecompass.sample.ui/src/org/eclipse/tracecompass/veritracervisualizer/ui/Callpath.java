package org.eclipse.tracecompass.veritracervisualizer.ui;

import org.eclipse.swt.widgets.TreeItem;

/**
 * Represent a callpath item and what's needed to use them in the view
 * @author Damien Thenot
 *
 */
public class Callpath{
	
	//Root attribute in the statesystem
	public static final String EventRoot = "callpath";
	//Event name in the CTF traces
	public static final String EventName = "callpath";
	
	private int parent; //The parent's id from the trace
	private int id; //This object's identifier with other callpath object
	private int quark; //This object's identifier  in the state system
	private String name; //This object's name, it is what's shown on the tree view on the left of the Global View
	private TreeItem tree; //The treeitem representing this object in the tree view
	
	
	public Callpath(int parent, String name, int id, int quark) {
		this.parent = parent;
		this.name = name;
		this.id = id;
		this.quark = quark;
	}
	
	public int getQuark() {
		return quark;
	}
	
	public int getId() {
		return id;
	}
	
	
	public String getName() {
		return name;
	}
	
	public void setName(String name) {
		this.name = name;
	}
	
	public String toString() {
		return name + " : ID : " + id + ", Parent : " + parent;
	}

	public int getParent() {
		return parent;
	}

	public TreeItem getTree() {
		return tree;
	}

	//Allow to set the tree of this object after it has been created
	public void setTree(TreeItem tree) {
		this.tree = tree;
		tree.setText(name);
		tree.setExpanded(true);
	}
}