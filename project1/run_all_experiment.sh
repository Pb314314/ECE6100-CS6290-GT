for script in valid_grad_*.sh; do
    ./"$script" &
done

wait