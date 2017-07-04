package com.picklingtools.pythonesque;

// Simple wrapper for a complex array.  Contains data by float[], but
// twice as much as real, imag, real, imag, etc.

public class Complex8Array extends Object {
    public Complex8Array(float[] real_imag_data) {
	data_ = real_imag_data;
    }
    public float [] data_;

}; // Complex8Array