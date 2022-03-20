# Projet Systèmes et réseaux : Serveur HTTP

## Pré-requis :
- Installer Make
- Système d'exploitation Linux

## Configuration
```make mrproper``` : supprimer tous les fichiers générés

```make build``` : créer les dossiers obj, lib et bin

## Éxécution
```make server``` : compiler et éxécuter un server

Le client se connecte sur son navigateur à l'adresse ```http://localhost:8080/```
Il peut ensuite envoyer des requêtes GET comme ceci ```http://localhost:8080/http.png```

## Test
```make test_server```
```make test_client```
