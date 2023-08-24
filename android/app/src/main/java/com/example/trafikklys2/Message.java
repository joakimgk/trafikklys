package com.example.trafikklys2;

import java.sql.Timestamp;

public class Message {

    int clientID;
    byte[] data;
    Timestamp received;

    public Message(int ID, byte[] d) {
        clientID = ID;
        data = d;
        received = new Timestamp(System.currentTimeMillis());
    }
}
