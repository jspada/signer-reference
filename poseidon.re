open Core_kernel;

/* This hash function is instantiated using the [poseidon hash function](https://eprint.iacr.org/2019/458).
     It has a few parameters

     - SBOX
     - state size
     - round constants
     - MDS matrix
     - number of full rounds
     - number of partial rounds.

     At launch we will be using
     - SBOX: x -> x^5
     - state size: 3
     - number of full rounds: 63
     - number of partial rounds: 0

     See params.sage for how the round constants and mds matrix are generated.
   */

module Field = Snarkette.Tweedle.Fq;

let full_rounds = 63;

/*
     round_constants is a 63×3 matrix of Fq elements
     mds_matrix is a 3×3 matrix of Fq elements
 */

/* Returns x^5 */
let sbox = x => Field.(square(square(x)) * x);

let params = Params.params_Tweedle_q;

let mds = Array.map(~f=Array.map(~f=Field.of_string), params.mds);

let round_constants =
  Array.map(~f=Array.map(~f=Field.of_string), params.round_constants);

/* Matrix multiply */
let apply_mds = s => {
  let inner_product = (v1, v2) =>
    Array.reduce_exn(~f=Field.(+), Array.map2_exn(v1, v2, ~f=Field.( * )));

  Array.init(3, ~f=row => inner_product(mds[row], s));
};

/* state is an array of length 3 */
let poseidon_permutation = (state: array(Field.t)) => {
  let sbox = () =>
    for (i in 0 to 2) {
      state[i] = sbox(state[i]);
    };

  let ark = r =>
    for (i in 0 to 2) {
      state[i] = Field.(+)(state[i], round_constants[r][i]);
    };

  let mds = () => {
    let new_state = apply_mds(state);
    for (i in 0 to 2) {
      state[i] = new_state[i];
    };
  };

  ark(0);
  for (i in 0 to full_rounds - 1) {
    sbox();
    mds();
    ark(i + 1);
  };
};

let add_chunk = (state, x0, x1) => {
  state[0] = Field.(state[0] + x0);
  state[1] = Field.(state[1] + x1);
};

/* init is an array of length 3 */
let hash_state = (~init: array(Field.t), input: array(Field.t)) => {
  /* input is processed two entries at a time. */
  let state = Array.copy(init);
  let n = Array.length(input);
  let num_chunks = (n + 1) / 2;
  /* ceil(n / 2.0) */
  for (i in 0 to num_chunks - 1) {
    let x0 = input[2 * i];
    let x1 =
      if (2 * i + 1 < n) {
        input[2 * i + 1];
      } else {
        Field.zero;
      };
    /* To handle the case that the input has length not a multiple of 2 */
    add_chunk(state, x0, x1);
    poseidon_permutation(state);
  };
  state;
};

let digest = (~init: array(Field.t), input: array(Field.t)) => {
  let state = hash_state(~init, input);
  state[0];
};
