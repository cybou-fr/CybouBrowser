# Étude de cas — CYBOU Browser

**Projet open source personnel de R&D, conçu et développé par un ingénieur logiciel indépendant.**  
**Statut :** source 0.4.0-alpha.1, développement Windows, sans version publique qualifiée pour un usage quotidien sensible.

## 1. Problème et objectif

Les navigateurs modernes gèrent de nombreuses capacités sensibles (caméra, microphone, géolocalisation, notifications, presse-papiers, stockage, réseau), mais l'utilisateur ne distingue pas toujours la demande du site, la politique active, une autorisation accordée ou une utilisation effective.

**Objectif produit :** rendre ces capacités visibles et contrôlables grâce à une barre compacte, toujours accessible, sans présenter des états d'interface comme des garanties de sécurité non vérifiées.

## 2. Architecture retenue

- **C++20 et Qt WebEngine :** contrôleurs natifs, gestion du cycle de vie des documents et des permissions, politiques de navigation.
- **Qt Quick / QML :** onglets, navigation, barre de onze contrôles, réglages et panneaux contextuels.
- **Séparation des responsabilités :** le `PermissionCoordinator` rattache une demande native au contexte du document, de l'origine et du profil ; l'interface restitue l'état et transmet les actions explicites de l'utilisateur.
- **Services cloud :** catalogue indépendant du fournisseur ; les services sont ouverts comme des sites web ordinaires, sans accès privilégié à leurs comptes.
- **Stratégie sécurité :** modèle de menaces, décisions d'architecture, tests locaux et critères de publication fondés sur des preuves.

[Vue d'architecture](ARCHITECTURE_OVERVIEW.md) · [Décisions techniques](ENGINEERING_DECISIONS.md)

## 3. Réalisation observable dans les sources

- Navigation avec onglets, favoris, historique, téléchargement et navigation privée.
- Barre native/QML avec onze contrôles de capacités, observation des demandes et règles de décision liées aux contextes.
- Stratégies HTTPS et exceptions explicites limitées, observation réseau et filtrage local limité.
- Sélecteur de services cloud et interface de nouvel onglet.
- Suites de tests natifs/intégration et documentation détaillée des exigences et limites.

Le dépôt contient une [capture de l'interface réelle](screenshots/cybou-current-ui.webp). Le projet reste **en cours de qualification** : certains scénarios critiques, dont la révocation matérielle complète, le sandbox en distribution et les attaques réseau avancées, ne sont pas encore démontrés pour une sortie 1.0.

## 4. Exemples de décisions d'ingénierie

| Situation | Décision | Compromis assumé |
| --- | --- | --- |
| Contrôle des permissions | Coordination C++ sur demande native et contexte du document | Scénarios complexes iframe/navigation à vérifier |
| Transparence UX | Afficher séparément demande, politique, autorisation et blocage confirmé | L'utilisateur peut voir un état « inconnu » |
| Extensibilité | Aucune plateforme d'extensions tierces | Moins de fonctionnalités externes, surface privilégiée réduite |
| Services cloud | Raccourcis et fournisseurs configurables, sans privilèges | Pas d'intégration OAuth/service native implicite |
| Qualité | Tests et critères de publication explicites | Pas de promesse de sécurité avant qualification |

## 5. Ce que ce projet démontre

**Compétences illustrées :** architecture logicielle et systèmes, développement natif C++/Qt, interfaces QML, conception par frontières de confiance, gestion des dépendances, documentation et ingénierie de validation.

Il démontre une approche de **conception → architecture → implémentation → validation**, avec des écarts assumés entre la conception cible, le code présent et les garanties effectivement vérifiées.

Il **ne constitue pas**, à lui seul, un cas client ni une preuve de migration d'une stack vers une autre. Cette expertise est présentée séparément dans le profil professionnel.

## 6. Pour un responsable technique ou un client

L'intérêt du projet n'est pas le nombre de lignes de code : c'est la possibilité d'examiner les choix, les limites, les composants, les tests et les arbitrages d'un vrai produit.

[Code source et captures](../README.md) · [Documentation complète](DOCUMENTATION_INDEX.md) · [Qualité et preuves](ENGINEERING_QUALITY.md) · [État des validations](VALIDATION.md)

**Licence du code original : Apache-2.0.** Les composants tiers et visuels restent soumis à leurs droits respectifs ; voir [notices](../THIRD_PARTY_NOTICES.md).
