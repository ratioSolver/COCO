package it.cnr.coco.api;

import java.util.List;
import retrofit2.Call;
import retrofit2.http.GET;
import it.cnr.coco.api.Type;
import it.cnr.coco.api.Item;

public interface CoCoService {

    @GET("types")
    Call<List<Type>> get_types();

    @GET("types/{name}")
    Call<Type> get_type(String name);

    @GET("items")
    Call<List<Item>> get_items();

    @GET("items/{id}")
    Call<Item> get_item(String id);
}
