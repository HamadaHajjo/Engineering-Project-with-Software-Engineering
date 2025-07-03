package Model;

public class GameObject {
    private final String type;
    private final int id;
    private int row;
    private int col;

    public GameObject(String type, int id, int row, int col) {
        this.type = type;
        this.id = id;
        this.row = row;
        this.col = col;
    }


    public String getType() {
        return type;
    }

    public int getId() {
        return id;
    }

    public int getRow() {
        return row;
    }

    public int getCol() {
        return col;
    }

    public void setPosition(int row, int col) {
        this.row = row;
        this.col = col;
    }
}
