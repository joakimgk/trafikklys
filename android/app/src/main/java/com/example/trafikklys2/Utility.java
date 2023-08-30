package com.example.trafikklys2;

import android.util.Pair;

import java.util.ArrayList;

public class Utility {
    public static ArrayList<Client> clients = new ArrayList<Client>();

    public static Client _mapClient;

    public static ArrayList<Message> messages = new ArrayList<Message>();

    public static ArrayList<Client> verticalGroup = new ArrayList<Client>();

    public static ArrayList<Pair<Client, Client>> horizontalPairs = new ArrayList<Pair<Client, Client>>();

    public static void updateGroups() {
        // vertical units
        for (Client c : clients) {
            if (c.mTrafficLight.getCellOrientation() % 180 == 0) {
                verticalGroup.add(c);
            }
        }
        // horizontal pairs (may overlap)
        Client prev = null;
        for (int i = 0; i < clients.size(); i++) {
            Client c = clients.get(i);
            if (prev != null) {
                if (c.mTrafficLight.getCellOrientation() % 180 == 90) {
                    if (prev.mTrafficLight.getCellOrientation() % 180 == 90) {
                        if (c.mTrafficLight.getCellOrientation() != prev.mTrafficLight.getCellOrientation()) {
                            horizontalPairs.add(new Pair(prev, c));
                        }
                    }
                }
            }
            prev = c;
        }
    }
}
