package com.picklingtools.pythonesque;

// Simple wrapper for a complex array.  Contains data by double[], but
// twice as much as real, imag, real, imag, etc.

public class Complex16Array {
    public Complex16Array(double[] real_imag_data) {
	data_ = real_imag_data;
    }
    public double [] data_;

}; // Complex16Array