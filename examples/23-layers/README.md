# Contact layers: households, workplaces and public spaces

One SEIR model whose contacts come from three layers, each the kind that fits
the setting:

- **household**: a within-group mixing layer. Agents live in households of
  four and meet each housemate with probability 0.5 a day.
- **work**: a network layer, a small world redrawn at every run. The same
  coworkers every day.
- **public**: a mixing layer with a contact matrix across three age groups,
  drawn at random. Strangers, met afresh every day, at a rate set by the
  model parameter "Public contact rate".

The model combines the contacts of every layer in one draw per susceptible
agent, and records the layer of each transmission. The program prints the
model and the share of transmissions from each layer.
