package com.example.firemansimulator.controller;

import com.example.firemansimulator.model.MovementState;
import com.example.firemansimulator.view.viewcontroller.SceneController;
import javafx.stage.Stage;

public class MainController {

    private SceneController sceneController;
    private SimulationController simulationController;

    public MainController(Stage stage) {

        this.sceneController = new SceneController(stage, this);
        this.simulationController = new SimulationController(this);

    }

    public SceneController getSceneController() {
        return sceneController;
    }

    public SimulationController getSimulationController() {
        return simulationController;
    }
}
