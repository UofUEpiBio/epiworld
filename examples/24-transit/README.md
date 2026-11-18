# Contact layers with schedules and persistence: transit

The model of `23-layers` (households, workplaces, public spaces) with two
more layers and routine schedules:

- **work** samples contacts on weekdays only (`set_schedule()`: five days on,
  two off).
- **transit am** and **transit pm**: within-group mixing layers whose groups
  are trips (8 per route, 50 riders each) and whose pools are the 25 routes
  (`set_pools()`). On a weekday, a rider takes its usual trip with
  probability 0.8, and otherwise another trip of its route
  (`set_persistence(0.8)`): mostly the same crowd, sometimes another, as
  commuters do. Half the agents commute.
- **public** meets more people on weekends: its contact rate is a model
  parameter that follows a weekly cycle,
  `set_param_cycle("Public contact rate", {4, 4, 4, 4, 4, 6, 6})`.

A day off the schedule is a day without contacts in that layer, as a closure
is, but a schedule is a setting made between runs, while `close()` is the
one-off intervention a global event makes during a run.

The program prints the model and the share of transmissions from each
layer, on weekdays and weekends: work and transit transmit on weekdays only,
and public spaces carry more of the weekend transmissions.
