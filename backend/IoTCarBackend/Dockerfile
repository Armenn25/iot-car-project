# 1. Build stage: koristi .NET SDK za build
FROM mcr.microsoft.com/dotnet/sdk:8.0 AS build
WORKDIR /source

# Kopiraj .csproj i restore pakete
COPY *.csproj .
RUN dotnet restore

# Kopiraj ostatak koda i buildaj aplikaciju
COPY . .
RUN dotnet publish -c Release -o /app/publish

# 2. Runtime stage: koristi manju ASP.NET sliku
FROM mcr.microsoft.com/dotnet/aspnet:8.0
WORKDIR /app
COPY --from=build /app/publish .

# Otvori port na kojem će aplikacija raditi u containeru
EXPOSE 8080
ENV ASPNETCORE_URLS=http://+:8080

# Pokreni aplikaciju
ENTRYPOINT ["dotnet", "IoTCarBackend.dll"]
