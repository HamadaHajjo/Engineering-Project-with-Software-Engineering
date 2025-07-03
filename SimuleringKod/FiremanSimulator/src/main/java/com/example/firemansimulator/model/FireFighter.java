
package com.example.firemansimulator.model;

import com.example.firemansimulator.model.imu.Accelerometer;

import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

public class FireFighter {

    private int x;
    private int y;
    private double velocity;
    private int direction;

    private long lastUpdateTime = System.currentTimeMillis();
    private int lastX = x;
    private int lastY = y;
    private double lastVelocity = -1;
    private ScheduledExecutorService executor;
    private Accelerometer accelerometer;


    public FireFighter(int x, int y) {
        this.x = x;
        this.y = y;
        this.direction = 0;

        accelerometer = new Accelerometer(this);

        startSpeedCalculation();

    }

    private void startSpeedCalculation() {
        executor = Executors.newScheduledThreadPool(1);
        executor.scheduleAtFixedRate(() -> calculateSpeedContinuously(), 0, 1, TimeUnit.SECONDS);
    }

    public void calculateSpeedContinuously() {
        long currentTime = System.currentTimeMillis();
        int deltaX = x - lastX;
        int deltaY = y - lastY;

        double distance = Math.sqrt(deltaX * deltaX + deltaY * deltaY);
        double timeElapsed = (currentTime - lastUpdateTime) / 1000.0;

        if (timeElapsed > 0) {
            this.velocity = distance / timeElapsed;

            if (this.velocity != lastVelocity) {
                System.out.println("Current speed: " + String.format("%.2f", velocity) + " units per second");
                lastVelocity = this.velocity;
            }

        }
        lastX = x;
        lastY = y;
        lastUpdateTime = currentTime;
    }


    public void movement(MovementState[] movements) {
        if (movements == null || movements.length == 0) {
            System.out.println("No movements.");
            return;
        }

        int index = 0;
        MovementState currentState = movements[index];
        boolean isRunning = true;

        while (isRunning) {
            switch (currentState) {
                case FORWARD:
                    moveForward();
                    System.out.println("Moved forward to: (" + getX() + ", " + getY() + ") facing " + direction + " degrees");
                    break;

                case RIGHT:
                    turnRight();
                    System.out.println("Turned right. Now facing " + direction + " degrees");
                    break;

                case LEFT:
                    turnLeft();
                    System.out.println("Turned left. Now facing " + direction + " degrees");
                    break;

                case STOP:
                    System.out.println("Movement finished.");
                    isRunning = false;
                    break;
            }

            index++;
            if (index < movements.length) {
                currentState = movements[index];
            } else {
                isRunning = false;
            }
        }
    }

    private void moveForward() {
        switch (direction) {
            case 0:
                x++;
                break;
            case 90:
                y++;
                break;
            case 180:
                x--;
                break;
            case 270:
                y--;
                break;
        }
    }

    private void turnRight() {
        direction = (direction + 90) % 360;
    }

    private void turnLeft() {
        direction = (direction + 270) % 360;
    }

    public double getVelocity() {
        return velocity;
    }

    public void setVelocity(double velocity) {
        this.velocity = velocity;
    }

    public int getX() {
        return x;
    }

    public int getY() {
        return y;
    }

    public int getDirection() {
        return direction;
    }

    public void setX(int x) {
        this.x = x;
    }

    public void setY(int y) {
        this.y = y;
    }

    public void setDirection(int direction) {
        this.direction = direction;
    }

    public Accelerometer getAccelerometer() {
        return accelerometer;
    }
}
