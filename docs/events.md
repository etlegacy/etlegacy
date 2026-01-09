There's 2 ways to pass events from server to clients:

**NOTE**: Independent of which way you chose, entities have a big entityState
struct that can also be used to transmit info - it gets very tricky tho, because
you'd have to make sure that for the scenario of the event no other event is
trying to make use of that field itself at the same frame time.

# Plain Events (G_AddEvent)
- Slim on the wire.
- Attached to an already existing Entity.
- Can pass at max 1 event param.
- Up to 4 events can be transmitted per frame, any more than that and they will
  "silently" (can be logged) be dropped.

# Event Entities (G_TempEntity)
- Fatter on the wire
- Creates a new separate Entity with the lifetime of a single frame.
- Can pass multiple parameters (a temp event entity will have it's own
  entityState and any of it's fields can be used).
- There's an upper limit for how many entities can exist per snapshots (512),
  but as temp entitys are cleared on the next frame it's unlikely to be a
  problem unless you are bursting dozens or hundreds of additional entities.
