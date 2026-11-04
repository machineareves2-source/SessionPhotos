# Validation — Session Photos 0.1

4 novembre 2026.

## Compilation Windows validée sur GitHub Actions

La compilation Windows x64 et la suite C++ `state_and_network` ont réussi avec Visual Studio sur le commit `a0553ce1292d0e2ed941699d6a8e0d020df7d3a7`.

[Compilation et tests réussis](https://github.com/machineareves2-source/SessionPhotos/actions/runs/41908914379). Le paquet `SessionPhotos-Windows-VST3` contient le VST3 Windows compilé, son mode d'emploi, les licences et le lien vers les sources correspondantes.

## Vérifié dans l'environnement de développement

- Compilation native Linux du plugin VST3, avec JUCE 8.0.6 et GCC.
- Tests C++ de sauvegarde/restauration : octets des images, identifiants, notes en UTF-8, rotations et état vide conservés.
- État tronqué refusé, sans remplacement partiel des photos existantes.
- JPEG/PNG invalides et dimensions excessives refusés avant décodage ; limite de 64 photos vérifiée.
- Serveur HTTP réel : réception d'image, accusé de réception après insertion, galerie, téléchargement d'image à l'identique.
- Image reçue par HTTP puis sauvegardée et restaurée dans un nouveau magasin, sans fichiers externes.
- URL incorrecte et origine web étrangère refusées ; nouveau jeton de connexion après redémarrage.
- Interface mobile testée avec Chromium en viewport 390 × 844 : choix de fichier, conversion JPEG, redimensionnement 4000 × 2000 → 2400 × 1200, confirmation, galerie et absence de débordement horizontal.
- Parcours navigateur → vrai serveur C++ → galerie après rechargement, avec un titre français accentué.
- Fenêtre du plugin rendue et inspectée : champs, notes, QR code et lien sans chevauchement à la taille initiale 1140 × 820 ; encodage des accents corrigé.
- Syntaxe JavaScript vérifiée.

## À valider sur le matériel cible

- Chargement et scan du VST3 dans REAPER Windows.
- Sauvegarde, fermeture complète et réouverture d'un vrai projet REAPER ; test de duplication et de plusieurs instances.
- Autorisation du pare-feu, choix de la carte réseau, scan du QR code et prise de vue sur le Samsung.
- Lisibilité des petits repères après compression et ergonomie du zoom sur le matériel réel.

Les tests de sérialisation ne remplacent pas un test dans le DAW. Le code audio est un passage direct sans traitement ; les performances audio réelles dans la configuration du studio n'ont pas été mesurées.

## Relancer les tests

Tests C++ : `ctest --test-dir build -C Release --output-on-failure`.

Tests navigateur (optionnels) : installer Node.js et Playwright, puis `node Tests/browser/mobile.cjs`. La variable CHROME_PATH peut désigner un Chromium existant. Ce test utilise un serveur simulé pour les réponses HTTP.

Pour le test avec le serveur réel, lancer `PhotoTests --serve`, garder son entrée standard ouverte, puis exécuter `PHOTO_URL=<URL affichée> node Tests/browser/live.cjs`. Le serveur s'arrête à l'appui sur Entrée. Les deux programmes doivent partager le même réseau local/namespace.
