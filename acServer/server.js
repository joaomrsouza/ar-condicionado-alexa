const express = require("express");
const mongoose = require("mongoose");

mongoose.set("strictQuery", true);

const app = express();

const State = mongoose.model(
  "State",
  new mongoose.Schema({
    key: { type: String },
    powerState: { type: Boolean },
    fan: { enum: ["auto", "low", "medium", "high", "turbo"], type: String },
    temperature: { type: Number, min: 16, max: 30 },
    mode: { enum: ["heat", "cool", "dry", "fan", "auto"], type: String },
    swing: { type: Boolean },
    powerMode: { enum: ["eco", "quiet", "powerful"], type: String },
    display: { type: Boolean },
  })
);

function connectMongoDB() {
  mongoose.connect(process.env.MONGO_URI, {
    useNewUrlParser: true,
    useUnifiedTopology: true,
  });
  const db = mongoose.connection;
  db.on("error", console.error.bind(console, "connection error:"));
  db.once("open", async function () {
    console.log("Connected to MongoDB");
    await setupState();
    app.emit("ready");
  });
}

async function setupState() {
  console.log("Setting up state");
  const state = await State.findOne({ key: "arcondicionado" });
  if (!state) {
    console.log("Creating state");
    const state = new State({
      key: "arcondicionado",
      powerState: false,
      fan: "auto",
      temperature: 26,
      mode: "cool",
      swing: true,
      powerMode: "eco",
      display: true,
    });
    await state.save();
  } else {
    console.log("Found state");
  }
  console.log("State OK");
}

function authenticate(req, res, next) {
  if (req.headers.authorization !== process.env.AUTH_TOKEN) {
    res.status(401).send("Unauthorized");
    return;
  }
  next();
}

app.use(express.json());
connectMongoDB();

app.get("/", (_req, res) => {
  res.sendStatus(200);
});

// app.use(authenticate).get("/test", async (_req, res) => {
//   await State.findOneAndUpdate(
//     { key: "arcondicionado" },
//     { powerState: false }
//   );
//   res.sendStatus(200);
// });

app.use(authenticate).get("/esp", async (_req, res) => {
  const state = await State.findOne({ key: "arcondicionado" }).lean();
  delete state._id;
  delete state.key;
  delete state.__v;
  console.log("esp:", state);
  res.json(state);
});

app.use(authenticate).post("/alexa", async (req, res) => {
  await State.findOneAndUpdate({ key: "arcondicionado" }, req.body);
  console.log("alexa:", req.body);
  res.sendStatus(200);
});

app.on("ready", () => {
  app.listen(process.env.PORT || 3000, () => {
    console.log(`Server is running on port ${process.env.PORT || 3000}`);
  });
});
