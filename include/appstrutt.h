/* -------------------------------------------------------------------------
   =========================================================================
                            A P P S T R U T T . H
        Strutture utilizzate dall'applicativo.
   =========================================================================
   ------------------------------------------------------------------------- */ 
/* -----------------------------------------------------------------
        Struttura per appoggio informazioni assi
   ----------------------------------------------------------------- */         
struct wassi
{
        char    nome_asse[4] ;     	// Acronimo: M10 (Motore 10)
        char    descrizione[20] ;  	// es: Asse principale 
        char    marca[20] ; 	   	// es: SIEMENS - ecc
        char    modello[20] ;      	// es: MM4 - MM3 - ecc
        int     assex ;             // Indice AsseX nella dichiarazione assi.conf
        int     proto_asse ;        // Valore del protocollo
};

