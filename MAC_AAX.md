# Session Photos : Mac et AAX

## Mac (macOS 11 minimum)
Binaire universel Intel + Apple Silicon, formats VST3 et Audio Unit.
Fermer le DAW avant installation.
Copier Session Photos.vst3 dans ~/Library/Audio/Plug-Ins/VST3/
Copier Session Photos.component dans ~/Library/Audio/Plug-Ins/Components/
Créer les dossiers au besoin, puis relancer le DAW et son scan de plugins.

Cette version de développement utilise une signature locale ad hoc, sans certificat Apple Developer ID ni notarisation. macOS peut donc en bloquer le chargement après téléchargement. Aucun essai dans un DAW Mac réel n'a encore été effectué.
Si macOS bloque le plugin, après vérification de sa provenance, retirer uniquement sa quarantaine :
```sh
xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/VST3/Session Photos.vst3"
xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/Components/Session Photos.component"
```
Autoriser l'accès au réseau local si macOS le demande pour le transfert depuis le téléphone.

## AAX : préparation de compilation
Le projet accepte un SDK AAX fourni par un développeur autorisé :
```sh
cmake -S . -B build-aax -DSESSIONPHOTOS_AAX_SDK=/chemin/vers/AAX_SDK
cmake --build build-aax --config Release
```
Sous Windows, ajouter -A x64 à la configuration.
Cette option n'a pas été compilée ni testée faute de SDK AAX dans cet environnement.
Un binaire AAX non signé ne constitue pas une livraison utilisable dans Pro Tools standard. Il faut obtenir auprès d'Avid/PACE les outils et autorisations de signature appropriés, puis signer et tester la version finale. Pour les essais sans signature, utiliser la version développeur de Pro Tools prévue par Avid.
Informations : https://developer.avid.com/aax/
