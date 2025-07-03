package com.example.firemansimulator.model.imu;

import com.example.firemansimulator.model.FireFighter;

import java.util.Random;

public class Accelerometer {
    private FireFighter fireFighter;
    private double acceleration = 0.0;
    private double lastVelocity = 0.0;
    private double calculatedVelocity = 0.0;
    private long lastUpdateTime = System.currentTimeMillis();

    public Accelerometer(FireFighter fireFighter) {
        this.fireFighter = fireFighter;
        startAccelerometer();
    }

    private void updateAcceleration() {
        double currentVelocity = fireFighter.getVelocity();
        long currentTime = System.currentTimeMillis();
        double timeElapsed = (currentTime - lastUpdateTime) / 1000.0;

        if (timeElapsed > 0) {
            acceleration = (currentVelocity - lastVelocity) / timeElapsed;
        }

        lastVelocity = currentVelocity;
        lastUpdateTime = currentTime;
    }

    private void startAccelerometer() {
        Thread accelerationThread = new Thread(() -> {
            double lastAcceleration = acceleration;

            while (true) {
                updateAcceleration();
                if (acceleration != lastAcceleration) {
                    System.out.println("Current acceleration: " + String.format("%.2f", acceleration) + " m/s^2");
                    lastAcceleration = acceleration;
                }

                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    break;
                }
            }
        });

        accelerationThread.setDaemon(true);
        accelerationThread.start();
    }

    public double getAcceleration() {
        return acceleration;
    }

    private double calculateVelocity() {

        return calculatedVelocity;
    }
}