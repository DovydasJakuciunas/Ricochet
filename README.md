# Ricochet - Dogfight Game

- 2 Player PVP Dogfight game
- Play as a jet and attempt to take down your friend for victory. Collect upgrades and health to stay alive longer than your enemy.
- Shoot bullets and missiles to take them down.
- Restock your missiles using crates that spawn around the map.
- Avoid enemy gun fire to stay alive, you only get 3 chances to win. Good Luck out there.

  ## Client Server Plan for CA2
- For the multiplayer intergariton, I will be using the code from class. Not everything from the class code will be used as I've rewritten it to match the vision of my game. I will not be needing enemy AI as all the entities in the game will be strickly player.

  ### Network Protocol
- My first step will be to introduce a common language the the clients and servers can communicate through. This will require me to implement a network protocol.
- Within the protocol it will deifne the packet identifiers using enums and specify the exact structure of the data contained inside each packet.
- Some of the examples would be kPlayerEvent, kUpdateClientState all the examples will be packed using sf::Packet.
- One of the main rules will be data sequencing. Where the client and server must write and read packet infromation in the same order.
- For Ricochet the protocol will mainly be responsible for synchronising the two aircraft and any active projectiles. This will ensure that both players see the same positions, rotations, projectile trajectories, deaths and scores.
- The server will hold the authhoritaive version of these values rather than trusting values sent directly by a client.
  ### Game Server
- Currently my GameState updates the local World, obtains its CommandQueue and sends input from both local players into that queue.
- For the multiplayer version I will introduce a GameServer class which will act as the authoritative simulation of the Ricochet match.
- The server will have reposnibilies such as Aircarft movement, Collision detection, Damage dealt, instead of the CommandQueue receiving commands directly from two keyboards on the same machine, the server will recieve player actions through network packets.
- For example, Client 1 could send a request saying that Player 1 is currently roatitng left. The server will verify that this client actually owns Player 1 and will then create the correct command for that aircraft.
  ### Network Node
- My current Scene Graph is mainly reponsible for organising and updating objects diaplayed in the game.
- For the multiplayer version I will introduce a NetworkNode that inherits from SceneNode.
- The Network Node will act as a connection between network messages and the exisitng Scene Graph, this will be used for network events which affect the world rather than only one local input command.
- Some examples would be Spawn a projectile, Start match, End match, this allows me to keep using the command architecture already present in Ricochet instead of rewriting the entire game specifically around networking.
  ### Multiplayer Game State
- Currently GameState coantis the local game world and handles input for both Player 1 and Player 2 on the same computer.
- For CA2 I will be introduceing a MultiplayerGameState, instead of controlling both aircraft locally, each instance of the game will only control the aircraft assinged to that client.
- Therefore it will have 2 main networking responsibilities, first capture the local players keyboard input and send relevant action to the server, secondly it will continually receive packets from the server and use them to update the client's local representaion.
  ### How the Workflow Will Look
1.  Handshake and Initialisation — TCP
  - Server will run on a host machine and listen for incoming clients.
  - When a clinet connects the server will provide a unique PlayerID.
  - Server will then send a kInitialState packet containing the information required to initialise  the match.
  - Once they successfully connect and load the requeired resources, the server can send a packet such as kStartGame.
2.  Client Input Capture - Client to Server
  - Currently Ricochet, converts player keyboard input into commands which are pushed into the local CommandQueue.
  - In the multiplayer version the GameState will intercept these player actions.
  - This would allow the server to know when an action begins and when it stops.
  - the client therefore telling the server what the player wants to do, rather than changing the world directly.
3. Server Processing and Authority
  - The GameServer will receive input packets from all connected clients.
  - Each packet will first be associated with the PlayerID owend by the connection.
  - It will the translate the network action back into the same type of command currently used by the game.
  - The server will then simulate the game. Which the client will not be deciding the result such as "Where an aircraft moves"
4. World Heartbeat - Server to Client using UDP
  - After updating the authoritiavite world, the server will periodically broadcast the current game state to all connected clients.
  - The packet will contain a sever tick number followed by the state of each aircraft.
  - The packet can then contain information for each active projectile such as ProjectileID, Rotation, clients can use these to identify which local projectile corresponds to which server projectile.
  - If one movement snapshot is lost, there is generally little value in requesting that old snapshot again because a newer server state should arrive shortly afterwards.
5. Local Reconciliation and Rendering - Client Side
  - Once the client receives the lates server snapshot, it will update its local SceneGraph.
  - The server will remain authoritative but directly setting every sprite to every newly received position could make movement look rough, because the rendering frame rate may be higher than the networking update rate.
  - I will therefore need to implement interpolation. For remote objects the client can keep both previous server position/rotation and latest server position/rotation and smoothly interpolate between them while rendering.
  - For the player's own aircraft I may later investigate client-side prediction and server reconciliation if network latency makes direct server controlled input feel unresponsive.
### Protocols Comparison
  #### TCP - Transmission Control Protocol
  - It provides reliable and ordered delivery. If information is lost during transmission, TCP handles retransmission and maintains the correct order of the data.
  - I will use TCP for information where losing a message could leave the client or server in an invalid game state.
  - For example if a client never received the packet assigning it PlayerID::kPlayer2, it would not know which aircraft it is supposed to control. Reliability is therefore more important than speed for this type of data.
  #### UDP - User Datagram Protocol
  - It focuses on low latency transmission and does not guarantee delivery or ordering. This makes it useful for data which changes very frequently.
  - I intend to use UDP primarily for the server's world state heartbeat. Such as Aircraft X/Y position.
  - If one of the snapshots contains a projectile that's lost, retransmitting that old snapshot may no longer be useful because the projectile may already have moved to another location.
  - Preventing older UDP packets form overwriting newer information, each snapshot can ignore any packet older than the newest snapshot it has already processed.
  
