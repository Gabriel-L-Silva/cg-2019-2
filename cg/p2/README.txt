Autores: Gabriel Lucas da Silva e Lucas Santana Escobar

Todos os itens pedidos foram implementados. Por�m, � necess�rio destacar alguns pontos sobre os itens A4. 

A4: A transla��o foi feita utilizando o transform da camera do editor, levando a mesma para a posi��o do objeto de foco(em rela��o ao eixo global). Ap�s isso
utilizou-se o m�todo de transla��o para modificar a posi��o em z da c�mera do editor, em rela��o ao seu eixo local. Por fim, fez-se o dimensionamento
da c�mera do editor utilizando o teorema de pit�goras, em rela��o a altura e largura do objeto de foco, com as informa��es de dimens�o do objeto e viewAngle.
Em rela��o � escala no eixo z, a rela��o com a corre��o necess�ria � linear. No final o offset em z ficou, pit�goras no eixo x + pit�goras no eixo y + escala no eixo z.