# TP Awale
Un jeu de Awale en C avec un architecture client-serveur, projet de programmation réseau en 4IFA à l'INSA de Lyon.  
Ce projet est testé sur Linux et macOS.  

## Table des matières
- [1. Compilation](#1-compilation)
- [2. Lancement](#2-lancement)
    + [Serveur](#serveur)
    + [Client](#client)
- [3. Utilisation et fonctionnalités](#3-utilisation-et-fonctionnalités)
    + [Commandes Générales](#commandes-générales)
        * [`/help`](#help)
        * [`/exit`](#exit)
        * [`/info <username>`](#info-username)
        * [`/bio <biography>`](#bio-biography)
    + [Interaction avec les autres joueurs](#interaction-avec-les-autres-joueurs)
        * [`/addfriend <username>`](#addfriend-username)
        * [`/removefriend <username>`](#removefriend-username)
        * [`/getfriends`](#getfriends)
        * [`/list`](#list)
- [Commandes liées au jeu](#commandes-liées-au-jeu)
    * [`/listgames`](#listgames)
    * [`/challenge <username>`](#challenge-username)
    * [`/accept <game_id>`](#accept-game_id)
    * [`/decline <game_id>`](#decline-game_id)
    * [`/move <game_id> <hole_number>`](#move-game_id-hole_number)
    * [`/history <game_id>`](#history-game_id)
    * [`/gameinfo <game_id>`](#gameinfo-game_id)
    * [`/forfeit <game_id>`](#forfeit-game_id)
    * [`/watch <game_id>`](#watch-game_id)
    * [`/unwatch <game_id>`](#unwatch-game_id)
    * [`/match`](#match)
    * [`/visibility <game_id> <visibility>`](#visibility-game_id-visibility)
- [Conclusion](#conclusion)

## 1. Compilation
Ce projet utilise [make](https://www.gnu.org/software/make/) pour la compilation.

```bash
# Compilation du serveur et du client
make all

# Compilation du serveur
make server

# Compilation du client
make client

# Nettoyage des fichiers générés
make clean
```

## 2. Lancement

### Serveur
```bash
# Lancement du serveur
./server

# Pour compiler et lancer le serveur
make run-server
```

### Client
```bash
# Lancement du client vers localhost
./client
# En spécifiant l'adresse IP du serveur
./client <ip>

# Pour compiler et lancer le client
make run-client
```

## 3. Utilisation et fonctionnalités

Ce guide vous explique comment utiliser les commandes disponibles sur le serveur pour interagir avec d'autres utilisateurs et participer à des jeux. Ce système vous permet de gérer vos amis, de jouer à des jeux et de contrôler les informations associées à votre compte.

### Commandes Générales

##### `/help`
- **Description**: Affiche le message d'aide avec une liste complète des commandes disponibles.
    - **Exemple**:
    <img width="698" alt="Capture d’écran 2024-10-20 à 13 16 50" src="https://github.com/user-attachments/assets/fa082c34-a486-473a-8c6b-a79bb88ae37c">

##### `/exit`
- **Description**: Déconnecte l’utilisateur du serveur.

##### `/info <username>`
- **Description**: Récupère des informations sur un utilisateur, telles que son nom et sa biographie.
- **Paramètre**: 
    - `<username>` : Le nom d’utilisateur dont vous voulez connaître les informations.
- **Exemple**:
`/info JohnDoe`: Cela affichera les informations de l’utilisateur JohnDoe.

##### `/bio <biography>`
- **Description**: Définit ou modifie votre biographie.
- **Paramètre**:
    - `<biography>` : Le texte que vous souhaitez définir comme biographie.
- **Exemple**:
`/bio Salut!`: Cela définira votre biographie comme "Salut!".

### Interaction avec les autres joueurs

##### `/addfriend <username>`
- **Description**: Ajoute un utilisateur à votre liste d’amis. Cela lui permettra de voir vos parties privées.
- **Paramètre**:
    - `<username>` : Le nom d’utilisateur de la personne à ajouter.
- **Exemple**:
`/addfriend JaneDoe`: Cela ajoutera JaneDoe à votre liste d’amis.

##### `/removefriend <username>`
- **Description**: Supprime un utilisateur de votre liste d’amis.
- **Paramètre**:
    - `<username>` : Le nom d’utilisateur de la personne à retirer.
- **Exemple**:
`/removefriend JaneDoe`: Cela retirera JaneDoe de votre liste d’amis.

##### `/getfriends`
- **Description**: Affiche la liste de vos amis.

##### `/list`
- **Description**: Affiche la liste des utilisateurs actuellement connectés.

## Commandes liées au jeu
##### `/listgames`
- **Description**: Affiche la liste de tous les parties actives auxquels vous participez.

##### `/challenge <username>`
- **Description**: Défie un autre utilisateur à une partie.
- **Paramètre**:
    - `<username>` : Le nom d’utilisateur de la personne que vous souhaitez défier.
- **Exemple**:
`/challenge JaneDoe`: Cela défiera JaneDoe à une partie.

##### `/accept <game_id>`
- **Description**: Accepte un défi de partie.
- **Paramètre**:
    - `<game_id>` : L’identifiant de la partie que vous souhaitez accepter.
- **Exemple**:
`/accept 12345`: Cela acceptera le défi de la partie avec l’identifiant 12345.

##### `/decline <game_id>`
- **Description**: Refuse un défi de partie.
- **Paramètre**:
    - `<game_id>` : L’identifiant de la partie que vous souhaitez refuser.
- **Exemple**:
`/decline 12345`: Cela refusera le défi de la partie avec l’identifiant 12345.

##### `/move <game_id> <hole_number>`
- **Description**: Fait un mouvement dans une partie spécifique.
- **Paramètres**:
    - `<game_id>` : L’identifiant de la partie.
    - `<hole_number>` : Le numéro du trou où vous souhaitez jouer.
- **Exemple**:
`/move 12345 3`: Cela fera un mouvement dans la partie avec l’identifiant 12345 dans le trou 3.
            
##### `/history <game_id>`
- **Description**: Affiche l'historique des mouvements d'une partie spécifique.
- **Paramètre**:
    - `<game_id>` : L’identifiant de la partie.
- **Exemple**:
`/history 12345`: Cela affichera l'historique des mouvements de la partie avec l’identifiant 12345.

##### `/gameinfo <game_id>`
- **Description**: Affiche des informations détaillées sur une partie spécifique.
- **Paramètre**:
    - `<game_id>` : L’identifiant de la partie.
- **Exemple**:
`/gameinfo 12345`: Cela affichera des informations détaillées sur la partie avec l’identifiant 12345.
- **Important** : Si la partie est privé et que vous n'êtes **pas ami** avec le joueur, la commande échouera et vous recevrez un message d'erreur. Assurez-vous d'être ami avec le joueur ou que la partie soit publique.

##### `/forfeit <game_id>`
- **Description**: Abandonne une partie.
- **Paramètre**:
    - `<game_id>` : L’identifiant de la partie que vous souhaitez abandonner.
- **Exemple**:
`/forfeit 12345`: Cela abandonnera la partie avec l’identifiant 12345.

##### `/watch <game_id>`
- **Description**: Permet de regarder une partie en cours.
- **Paramètre**:
    - `<game_id>` : L’identifiant de la partie que vous souhaitez regarder.
- **Important** : Si la partie est privé et que vous n'êtes **pas ami** avec le joueur, la commande échouera et vous recevrez un message d'erreur précisant que vous ne pouvez pas observer la partie. Assurez-vous d'être ami avec le joueur ou que la partie soit public.
- **Exemple**:
`/watch 12345`: Cela vous permettra de regarder la partie avec l’identifiant 12345.

##### `/unwatch <game_id>`
- **Description**: Arrête de regarder une partie en cours.
- **Paramètre**:
    - `<game_id>` : L’identifiant de la partie que vous ne voulez plus regarder.
- **Exemple**:
`/unwatch 12345`: Cela arrêtera de regarder la partie avec l’identifiant 12345.

##### `/match`
- **Description**: Rejoint la file d'attente de matchmaking pour trouver un adversaire. Une fois qu'un adversaire est trouvé, une partie est créé et vous êtes invité à jouer.

##### `/visibility <game_id> <visibility>`
- **Description**: Définit la visibilité d'une partie en cours.
- **Paramètres**:
    - `<game_id>` : L’identifiant de la partie.
    - `<visibility>` : `0` pour une partie privé, `1` pour une partie public.
- **Exemple**:
`/visibility 12345 0`: Cela définira la partie avec l’identifiant 12345 comme privée.

## Conclusion
Ce guide couvre toutes les commandes disponibles pour naviguer et interagir avec les utilisateurs et les jeux sur le serveur. Utilisez ces commandes pour personnaliser votre expérience de jeu, gérer vos amis, et participer activement aux matchs.

Si vous essayez de regarder un jeu privé sans être ami avec le joueur principal, vous recevrez un message d'erreur. Assurez-vous que le jeu soit public ou que vous soyez dans la liste d'amis du joueur.

N'hésitez pas à consulter la commande `/help` si vous avez besoin d'un rappel sur les commandes à tout moment.
