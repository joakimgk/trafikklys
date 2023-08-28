package com.example.trafikklys2;

public class Program {

    public byte[] mProgram;

    public Client mClient;

    public Program(byte[] p) {
        mProgram = p;
    }

    public Program(byte[] p, Client c) {
        mProgram = p;
        mClient = c;
    }

}
