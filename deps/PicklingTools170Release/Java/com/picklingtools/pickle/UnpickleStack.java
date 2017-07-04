package com.picklingtools.pickle;

import java.io.Serializable;
import java.util.Collections;

import com.picklingtools.pythonesque.Arr;


/**
 * Helper type that represents the unpickler working stack.
 * 
 * @author Irmen de Jong (irmen@razorvine.net)
 */
public class UnpickleStack implements Serializable {
	private static final long serialVersionUID = 6148106506441423350L;
	private Arr stack;
	protected Object MARKER;

	public UnpickleStack() {
		stack = new Arr();
		MARKER = new Object(); // any new unique object
	}

	public void add(Object o) {
		this.stack.add(o);
	}

	public void add_mark() {
		this.stack.add(this.MARKER);
	}

	public Object pop() {
		int size = this.stack.size();
		Object result = this.stack.get(size - 1);
		this.stack.remove(size - 1);
		return result;
	}

	public Arr pop_all_since_marker() {
		Arr result = new Arr();
		Object o = pop();
		while (o != this.MARKER) {
			result.add(o);
			o = pop();
		}
		result.trimToSize();
		Collections.reverse(result);
		return result;
	}

	public Object peek() {
		return this.stack.get(this.stack.size() - 1);
	}

	public void trim() {
		this.stack.trimToSize();
	}

	public int size() {
		return this.stack.size();
	}

	public void clear() {
		this.stack.clear();
		this.stack.trimToSize();
	}
}
