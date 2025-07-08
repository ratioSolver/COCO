package it.cnr.coco;

import java.util.List;
import retrofit2.Call;
import retrofit2.http.GET;
import it.cnr.coco.model.Type;

public interface CoCoService {

    @GET("types")
    Call<List<Type>> get_types();

    @GET("types/{name}")
    Call<Type> get_type(String name);
}
