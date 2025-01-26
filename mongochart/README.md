##This is the extended modification to the original repository https://github.com/CrusaderX/mongodb-charts
---
## The changes made
1. We change the docker-compose file , from mapping the port, to Host network mode
2. We change the docker/chart/Dockerfile as the changes below
   We change the image tag and remove the line apt-get update
---
In accordance with https://docs.mongodb.com/charts/master/launch-charts/#launch-charts this repo exist for whom who want to run this stuff locally with docker-compose.

```yaml
version: '2'

services:
  charts:
    build:
      context: 'docker/charts'
      args:
        - EMAIL=admin@example.com
        - PASSWORD=admin123
    image: charts
    network_mode: host
    environment:
      CHARTS_SUPPORT_WIDGET_AND_METRICS: 'on'
      CHARTS_MONGODB_URI: 'mongodb://localhost:27017/admin?replicaSet=rs0'
    volumes:
      - keys:/mongodb-charts/volumes/keys
      - logs:/mongodb-charts/volumes/logs
      - db-certs:/mongodb-charts/volumes/db-certs
      - web-certs:/mongodb-charts/volumes/web-certs
    depends_on:
      - mongo
    container_name: charts

  mongo:
    hostname: localhost
    build:
      context: 'docker/mongo'
    network_mode: host
    volumes:
      - mongo:/data/db
    image: charts_mongo
    container_name: mongo

volumes:
  keys:
  logs:
  db-certs:
  web-certs:
  mongo:
~          
```
---
Just one step to run

```console
$ docker-compose up -d
```
If the command above fails 
```console
$ docker-compose up -d --build
```
---
After ~2 mins you should be able to open your browser and navigate to http://localhost:8080 and see the login menu. Login and password are described in compose file.
