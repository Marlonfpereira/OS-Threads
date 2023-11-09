
restaurant.cc              -> código paralelo com seções críticas protegidas e tempos de espera aleatórios (entre 1 e 4 segundos)
restaurant-unprotected.cc  -> código paralelo com seções críticas não protegidas (sem nenhum semáforo) e tempos de espera aleatórios (entre 1 e 4 segundos)
restaurant-serial.cc       -> código serial e tempos de espera aleatórios (entre 1 e 4 segundos)

restaurant-fixed.cc        -> código paralelo com seções críticas protegidas e tempos de espera fixos (1 segundo)
restaurant-serial-fixed.cc -> código serial e tempos de espera fixos (1 segundo)

Os códigos paralelos podem receber 3 parâmetros de terminal, configurando o número de clientes, cozinheiros e garçons, respectivamente.
Os códigos seriais recebem apenas um, o número de clientes.