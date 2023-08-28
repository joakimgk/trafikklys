package com.example.trafikklys2;

import java.util.Comparator;

public class ClientsComparator implements Comparator<Client> {

    @Override
    public int compare(Client o1, Client o2) {
        return o1.mTrafficLight.cellX > o2.mTrafficLight.cellX ? 1 : -1;
    }
}
