package com.example.trafikklys2;

import java.util.ArrayList;

public class Utility {

    private static ClientsAdapter mAdapter;

    public static ArrayList<Client> clients = new ArrayList<Client>();
    public static ArrayList<Message> messages = new ArrayList<Message>();

    public static void setAdapter(ClientsAdapter adapter) {
        mAdapter = adapter;
    }

    public static ClientsAdapter getAdapter() {
        return mAdapter;
    }
}
