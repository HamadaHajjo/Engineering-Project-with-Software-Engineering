package com.example.firemansimulator;

import com.example.firemansimulator.controller.MainController;
import javafx.application.Application;
import javafx.stage.Stage;

import java.io.IOException;

public class FiremanSimulator extends Application {

    @Override
    public void start(Stage stage) throws IOException {

        MainController mainController = new MainController(stage);
    }

    public static void main(String[] args) {
        launch();
    }
}