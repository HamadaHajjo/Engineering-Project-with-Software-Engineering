import org.json.JSONObject;

public class Main {
    public static void main(String[] args) {
        JSONObject json = new JSONObject();
        json.put("key", "value");
        System.out.println(json);
    }
}
