
package com.example.firemansimulator.view;

import javafx.scene.layout.GridPane;
import javafx.scene.paint.Color;
import javafx.scene.shape.Polygon;
import javafx.scene.shape.Rectangle;

public class Grid {
    private int rows;
    private int columns;
    private GridPane gridPane;
    private Polygon directionArrow;

    public Grid(int rows, int columns, GridPane gridPane) {
        this.rows = rows;
        this.columns = columns;
        this.gridPane = gridPane;

        initializeGrid();
        initializeDirectionArrow();
    }

    private void initializeGrid() {
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < columns; col++) {
                Rectangle square = new Rectangle(55, 35);
                square.setFill(Color.LIGHTGRAY);
                square.setStroke(Color.BLACK);
                gridPane.add(square, col, row);
            }
        }
    }

    private void initializeDirectionArrow() {
        directionArrow = new Polygon();
        directionArrow.getPoints().addAll(10.0, 0.0, -5.0, -15.0, -5.0, 15.0);

        directionArrow.setFill(Color.RED);
        directionArrow.setTranslateX((55 / 2.0) - 6);
        directionArrow.setTranslateY((35 / 2.0) - 17);

        gridPane.getChildren().add(directionArrow);
    }

    public void updateFirefighterPosition(int oldX, int oldY, int newX, int newY, int direction) {
        if (directionArrow != null && isValidPosition(oldX, oldY) && isValidPosition(newX, newY)) {
            GridPane.setColumnIndex(directionArrow, newX);
            GridPane.setRowIndex(directionArrow, newY);

            directionArrow.setRotate(direction);
        }
    }

    private boolean isValidPosition(int x, int y) {
        return x >= 0 && x < columns && y >= 0 && y < rows;
    }
}
