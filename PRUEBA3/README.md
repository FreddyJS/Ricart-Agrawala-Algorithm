# Ricart-Agrawala-Algorithm
C implementation of the tickets algorithm for a distributed system.

The program emulates the different nodes using message queues for communication

To test it with a lot of process we need to increase the max queue size:   

```sh
sudo sysctl kernel.msgmnb=2000000000
```
DESCRIPCIÓN: TIEMPO DE ENTRADA DESPUES DE UNA SALIDA DE PAGOS, (SOLO HAY DE PAGOS)