using IoTCarBackend.Hubs;

var builder = WebApplication.CreateBuilder(args);

builder.Services.AddControllers();
builder.Services.AddSignalR();
builder.Services.AddEndpointsApiExplorer();
builder.Services.AddSwaggerGen();
builder.Services.AddCors(options => {
    options.AddPolicy("AllowNetlify", builder =>
        builder
            .WithOrigins("https://iotcarc.netlify.app") // tvoj Netlify FE domen
            .AllowAnyHeader()
            .AllowAnyMethod()
            .AllowCredentials()
    );
});

var app = builder.Build();
app.UseCors("AllowNetlify");
app.UseSwagger();
app.UseSwaggerUI();


if (app.Environment.IsDevelopment())
{
    app.UseSwagger();
    app.UseSwaggerUI();
}


app.UseCors("AllowAngularApp");
app.UseAuthorization();
app.MapControllers();
app.MapHub<CarHub>("/carhub").RequireCors("AllowNetlify");
app.Run();
 