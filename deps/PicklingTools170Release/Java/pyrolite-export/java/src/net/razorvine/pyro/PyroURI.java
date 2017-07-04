package net.razorvine.pyro;

import java.io.IOException;
import java.io.Serializable;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import net.razorvine.pickle.Pickler;
import net.razorvine.pickle.Unpickler;
import net.razorvine.pickle.objects.AnyClassConstructor;

/**
 * The Pyro URI object.
 * 
 * @author Irmen de Jong (irmen@razorvine.net)
 */
public class PyroURI implements Serializable {

	private static final long serialVersionUID = -7611447798373262153L;
	String protocol = "PYRO";
	String objectid;
	String host;
	int port;

	static {
		Unpickler.registerConstructor("Pyro4.core", "URI", new AnyClassConstructor(PyroURI.class));
		Pickler.registerCustomPickler(PyroURI.class, new PyroUriPickler());
	}
	
	public PyroURI() {
	}

	public PyroURI(PyroURI other) {
		protocol = other.protocol;
		objectid = other.objectid;
		host = other.host;
		port = other.port;
	}

	public PyroURI(String uri) {
		Pattern p = Pattern.compile("(PYRO[A-Z]*):(\\S+?)(@(\\S+))?$");
		Matcher m = p.matcher(uri);
		if (m.find()) {
			protocol = m.group(1);
			objectid = m.group(2);
			String[] loc = m.group(4).split(":");
			host = loc[0];
			port = Integer.parseInt(loc[1]);
		} else {
			throw new PyroException("invalid URI string");
		}
	}

	public PyroURI(String objectid, String host, int port) {
		this.objectid = objectid;
		this.host = host;
		this.port = port;
	}

	public String toString() {
		return "<PyroURI " + protocol + ":" + objectid + "@" + host + ":" + port + ">";
	}


	@Override
	public int hashCode() {
		return toString().hashCode();
	}

	@Override
	public boolean equals(Object obj) {
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (!(obj instanceof PyroURI))
			return false;
		PyroURI other = (PyroURI) obj;
		return toString().equals(other.toString());
	}

	/**
	 * called by the Unpickler to restore state
	 */
	public void __setstate__(Object[] args) throws IOException {
		this.protocol = (String) args[0];
		this.objectid = (String) args[1];
		this.host = (String) args[3];
		this.port = (Integer) args[4];
	}
}
