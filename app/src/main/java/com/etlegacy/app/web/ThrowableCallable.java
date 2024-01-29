package com.etlegacy.app.web;

@FunctionalInterface
public interface ThrowableCallable<T> {
	void apply(T t) throws Exception;
}
