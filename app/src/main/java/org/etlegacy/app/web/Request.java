package org.etlegacy.app.web;

import java.util.HashMap;
import java.util.Map;

public class Request {
	public String url;
	public String authToken;
	public int requestType; // 0 = GET, 1 = POST
	public Map<String, String> headers = new HashMap<>();
	public long nativeIdentifier; // this links the C side of things

	public void addHeader(String name, String value) {
		if (name == null || name.trim().isEmpty()) {
			return;
		}

		if (value == null) {
			this.headers.remove(name);
			return;
		}
		this.headers.put(name, value);
	}
}
