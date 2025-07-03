
package com.example.firemansimulator.model.simulation;

import com.example.firemansimulator.controller.SimulationController;
import com.example.firemansimulator.model.FireFighter;
import com.example.firemansimulator.model.MovementState;
import javafx.application.Platform;

import java.util.Timer;
import java.util.TimerTask;

public class Simulation {

    private FireFighter fireFighter;
    private SimulationController simulationController;
    private double estimatedX = 0;
    private double estimatedY = 0;
    private double lastEstimatedX = 0;
    private double lastEstimatedY = 0;
    private double estimatedVelocity = 0;
    private long lastUpdateTime = System.currentTimeMillis();

    public Simulation(SimulationController simulationController) {
        this.simulationController = simulationController;
        fireFighter = new FireFighter(0, 0);

        startSimulation();

    }

    public void startSimulation() {
        Timer timer = new Timer();
        timer.scheduleAtFixedRate(new TimerTask() {
            @Override
            public void run() {
                updateEstimatedPosition();
                Platform.runLater(() -> simulationController.getEstimatedPositionGrid().updateFirefighterPosition(
                        (int) lastEstimatedX, (int) lastEstimatedY, (int) estimatedX, (int) estimatedY, fireFighter.getDirection()
                ));
            }
        }, 0, 100);
    }




    public void moveFireFighter(MovementState direction) {
        int oldX = fireFighter.getX();
        int oldY = fireFighter.getY();

        fireFighter.movement(new MovementState[] {direction});

        int newX = fireFighter.getX();
        int newY = fireFighter.getY();
        int currentDirection = fireFighter.getDirection();

        Platform.runLater(() -> simulationController.getRealPositionGrid().updateFirefighterPosition(oldX, oldY, newX, newY, currentDirection));
    }

    public void updateEstimatedPosition() {

        lastEstimatedX = estimatedX;
        lastEstimatedY = estimatedY;

        double acceleration = fireFighter.getAccelerometer().getAcceleration();
        long currentTime = System.currentTimeMillis();
        double timeElapsed = (currentTime - lastUpdateTime) / 1000.0;

        estimatedVelocity += acceleration * timeElapsed;

        double distance = estimatedVelocity * timeElapsed + 0.5 * acceleration * timeElapsed * timeElapsed;

        switch (fireFighter.getDirection()) {
            case 0:
                estimatedX += distance;
                break;
            case 90:
                estimatedY += distance;
                break;
            case 180:
                estimatedX -= distance;
                break;
            case 270:
                estimatedY -= distance;
                break;
        }

        lastUpdateTime = currentTime;
    }

}
