package com.example.firemansimulator.view.viewcontroller;

import com.example.firemansimulator.controller.MainController;
import javafx.event.EventHandler;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.scene.input.KeyEvent;
import javafx.stage.Stage;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

public class SceneController {

    private MainController mainController;
    private Stage stage;
    private Map<String, Scene> scenes = new HashMap<>();


    public SceneController(Stage stage, MainController mainController) {

        this.mainController = mainController;
        this.stage = stage;

        initializeScenes();
        activateScene("simulation");
        stage.setTitle("Fireman Simulation");
        stage.show();
    }

    private void initializeScenes() {

        addScene("simulation", "/com/example/firemansimulator/simulation.fxml");
    }

    public void addScene(String name, String fxmlPath) {
        try {
            FXMLLoader loader = new FXMLLoader(getClass().getResource(fxmlPath));
            Parent root = loader.load();
            Scene scene = new Scene(root);
            scenes.put(name, scene);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void activateScene(String name) {
        Scene scene = scenes.get(name);
        if (scene != null) {
            stage.setScene(scene);
        } else {
            System.out.println("Scene " + name + " not found!");
        }
    }

}
