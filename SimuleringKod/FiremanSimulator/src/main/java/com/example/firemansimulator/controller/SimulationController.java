package com.example.firemansimulator.controller;

import com.example.firemansimulator.model.MovementState;
import com.example.firemansimulator.model.simulation.Simulation;
import com.example.firemansimulator.view.Grid;
import javafx.fxml.FXML;
import javafx.scene.layout.GridPane;
import javafx.scene.input.KeyEvent;
import javafx.scene.input.KeyCode;

public class SimulationController {

    private MainController mainController;

    @FXML
    private GridPane realPositionPane;

    @FXML
    private GridPane estimatedPositionPane;

    private Grid realPositionGrid;
    private Grid estimatedPositionGrid;
    private Simulation simulation;

    // Konstruktorer
    public SimulationController(MainController mainController) {
        this.mainController = mainController;
    }

    public SimulationController() {}

    @FXML
    public void initialize() {
        realPositionGrid = new Grid(realPositionPane.getRowCount(), realPositionPane.getColumnCount(), realPositionPane);
        estimatedPositionGrid = new Grid(estimatedPositionPane.getRowCount(), estimatedPositionPane.getColumnCount(), estimatedPositionPane);

        simulation = new Simulation(this);

        realPositionPane.sceneProperty().addListener((observable, oldScene, newScene) -> {
            if (newScene != null) {
                newScene.setOnKeyPressed(this::handleKeyPress);
            }
        });
    }

    private void handleKeyPress(KeyEvent event) {
        KeyCode code = event.getCode();

        if (simulation != null) {
            if (code == KeyCode.UP) {
                simulation.moveFireFighter(MovementState.DOWN);
            } else if (code == KeyCode.SPACE) {
                simulation.moveFireFighter(MovementState.FORWARD);
            } else if (code == KeyCode.LEFT) {
                simulation.moveFireFighter(MovementState.LEFT);
            } else if (code == KeyCode.RIGHT) {
                simulation.moveFireFighter(MovementState.RIGHT);
            }
        }
    }

    public Grid getRealPositionGrid() {
        return realPositionGrid;
    }
    public Grid getEstimatedPositionGrid() {
        return estimatedPositionGrid;
    }
}
