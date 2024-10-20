# TP Awale

## 1. Build

## 2. Lancement

## 3. Utilisation et fonctionnalités
### Guide d'utilisateur pour le système de commandes

Ce guide vous explique comment utiliser les commandes disponibles sur le serveur pour interagir avec d'autres utilisateurs et participer à des jeux. Ce système vous permet de gérer vos amis, de jouer à des jeux et de contrôler les informations associées à votre compte.

#### Commandes Générales

##### `/help`
- **Description**: Affiche le message d'aide avec une liste complète des commandes disponibles.
- **Exemple**:

<img width="698" alt="Capture d’écran 2024-10-20 à 13 16 50" src="https://github.com/user-attachments/assets/fa082c34-a486-473a-8c6b-a79bb88ae37c">

##### `/exit`
- **Description**: Déconnecte l’utilisateur du serveur.
- **Exemple**:
Utilisez cette commande pour quitter la session en cours.

##### `/info <username>`
- **Description**: Récupère des informations sur un utilisateur, telles que son nom et sa biographie.
- **Paramètre**: 
- `<username>` : Le nom d’utilisateur dont vous voulez connaître les informations.
- **Exemple**:
Utilisez cette commande pour quitter la session en cours.

##### `/info <username>`
- **Description**: Récupère des informations sur un utilisateur, telles que son nom et sa biographie.
- **Paramètre**: 
- `<username>` : Le nom d’utilisateur dont vous voulez connaître les informations.
- **Exemple**:
Cela affichera les informations de l’utilisateur nommé JohnDoe.

##### `/bio <biography>`
- **Description**: Définit ou modifie votre biographie.
- **Paramètre**:
- `<biography>` : Le texte que vous souhaitez définir comme biographie.
- **Exemple**:
Cela mettra à jour votre biographie avec le texte fourni.

### Interaction avec les autres joueurs

##### `/addfriend <username>`
- **Description**: Ajoute un utilisateur à votre liste d’amis.
- **Paramètre**:
- `<username>` : Le nom d’utilisateur de la personne à ajouter.
- **Exemple**:
Cette commande ajoutera JaneDoe à votre liste d’amis.

##### `/removefriend <username>`
- **Description**: Supprime un utilisateur de votre liste d’amis.
- **Paramètre**:
- `<username>` : Le nom d’utilisateur de la personne à retirer.
- **Exemple**:
Cela retirera JaneDoe de votre liste d’amis.

##### `/getfriends`
- **Description**: Affiche la liste de vos amis.
- **Exemple**:
Cela vous montrera tous les utilisateurs que vous avez ajoutés en tant qu’amis.

##### `/list`
- **Description**: Affiche la liste des utilisateurs actuellement connectés.
- **Exemple**:
Vous pourrez voir tous les utilisateurs connectés au serveur en temps réel.

## Commandes liées aux jeux

##### `/listgames`
- **Description**: Affiche la liste de tous les jeux actifs auxquels vous participez.
- **Exemple**:
Cela affichera la liste des jeux en cours où vous êtes impliqué.

##### `/challenge <username>`
- **Description**: Défie un autre utilisateur à un jeu.
- **Paramètre**:
- `<username>` : Le nom d’utilisateur de la personne que vous souhaitez défier.
- **Exemple**:
Cela défiera JohnDoe à un jeu.

##### `/accept <game_id>`
- **Description**: Accepte un défi de jeu.
- **Paramètre**:
- `<game_id>` : L’identifiant du jeu que vous souhaitez accepter.
- **Exemple**:
Cela acceptera le défi du jeu avec l’identifiant 12345.

##### `/decline <game_id>`
- **Description**: Refuse un défi de jeu.
- **Paramètre**:
- `<game_id>` : L’identifiant du jeu que vous souhaitez refuser.
- **Exemple**:
Cela refusera le défi du jeu avec l’identifiant 12345.

##### `/move <game_id> <hole_number>`
- **Description**: Fait un mouvement dans un jeu spécifique.
- **Paramètres**:
- `<game_id>` : L’identifiant du jeu.
- `<hole_number>` : Le numéro du trou où vous souhaitez jouer (applicable à un jeu comme Puissance 4).
- **Exemple**:
Cela jouera dans la colonne 4 du jeu 12345.

##### `/history`
- **Description**: Affiche l'historique des mouvements du jeu en cours.
- **Exemple**:
Cela affichera les mouvements passés du jeu actuel.

##### `/gameinfo <game_id>`
- **Description**: Affiche des informations détaillées sur un jeu spécifique.
- **Paramètre**:
- `<game_id>` : L’identifiant du jeu.
- **Exemple**:
Cela vous donnera des détails sur le jeu ayant l’identifiant 12345.

##### `/forfeit <game_id>`
- **Description**: Abandonne un jeu.
- **Paramètre**:
- `<game_id>` : L’identifiant du jeu que vous souhaitez abandonner.
- **Exemple**:
Cela vous fera abandonner le jeu avec l’identifiant 12345.

##### `/watch <game_id>`
- **Description**: Permet de regarder un jeu en cours.
- **Paramètre**:
- `<game_id>` : L’identifiant du jeu que vous souhaitez regarder.
- **Important** : Si le jeu est privé et que vous n'êtes **pas ami** avec le joueur, la commande échouera et vous recevrez un message d'erreur précisant que vous ne pouvez pas observer le jeu. Assurez-vous d'être ami avec le joueur ou que le jeu soit public.
- **Exemple**:
Vous pourrez regarder le jeu avec l’identifiant 12345. Cependant, si ce jeu est privé et que vous n'êtes pas ami avec le joueur principal, vous recevrez un message d'erreur indiquant que vous n'avez pas les permissions nécessaires pour observer ce jeu.

##### `/unwatch <game_id>`
- **Description**: Arrête de regarder un jeu en cours.
- **Paramètre**:
- `<game_id>` : L’identifiant du jeu que vous ne voulez plus regarder.
- **Exemple**:
Cela vous retirera de la liste des spectateurs du jeu 12345.

##### `/match`
- **Description**: Rejoint la file d'attente de matchmaking pour trouver un adversaire.
- **Exemple**:
Cela vous mettra en file d'attente pour trouver un match contre un autre joueur.

##### `/visibility <game_id> <visibility>`
- **Description**: Définit la visibilité d'un jeu en cours.
- **Paramètres**:
- `<game_id>` : L’identifiant du jeu.
- `<visibility>` : `0` pour un jeu privé, `1` pour un jeu public.
- **Exemple**:
Cela rendra le jeu avec l’identifiant 12345 visible au public.

## Conclusion

Ce guide couvre toutes les commandes disponibles pour naviguer et interagir avec les utilisateurs et les jeux sur le serveur. Utilisez ces commandes pour personnaliser votre expérience de jeu, gérer vos amis, et participer activement aux matchs.

Si vous essayez de regarder un jeu privé sans être ami avec le joueur principal, vous recevrez un message d'erreur. Assurez-vous que le jeu soit public ou que vous soyez dans la liste d'amis du joueur.

N'hésitez pas à consulter la commande `/help` si vous avez besoin d'un rappel sur les commandes à tout moment.
